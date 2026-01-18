#include "aeon_monitor.h"
#include "dashboard_manager.h"
#include "chart/chart.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <time.h>

#define PORT 40005
#define BUFFER_SIZE 65536
#define DB_PATH "../data/aeon_evolution.db"

// HTTP 응답 헤더 생성
void send_http_response(int client_fd, int status_code, const char *content_type, const char *body) {
    char header[1024];
    const char *status_text = (status_code == 200) ? "OK" :
                              (status_code == 404) ? "Not Found" :
                              (status_code == 400) ? "Bad Request" : "Internal Server Error";

    snprintf(header, sizeof(header),
        "HTTP/1.1 %d %s\r\n"
        "Content-Type: %s\r\n"
        "Access-Control-Allow-Origin: *\r\n"
        "Content-Length: %ld\r\n"
        "Connection: close\r\n"
        "\r\n",
        status_code, status_text, content_type, strlen(body));

    write(client_fd, header, strlen(header));
    write(client_fd, body, strlen(body));
}

// JSON에서 문자열 값 추출
char* extract_json_string(const char *json, const char *key, char *out, size_t max_size) {
    char search[128];
    snprintf(search, sizeof(search), "\"%s\"", key);

    const char *start = strstr(json, search);
    if (!start) return NULL;

    start = strchr(start + strlen(search), ':');
    if (!start) return NULL;

    start = strchr(start, '"');
    if (!start) return NULL;
    start++;

    const char *end = strchr(start, '"');
    if (!end) return NULL;

    size_t len = end - start;
    if (len >= max_size) len = max_size - 1;

    strncpy(out, start, len);
    out[len] = '\0';
    return out;
}

// JSON에서 정수 값 추출
int extract_json_int(const char *json, const char *key) {
    char search[128];
    snprintf(search, sizeof(search), "\"%s\"", key);

    const char *start = strstr(json, search);
    if (!start) return 0;

    start = strchr(start + strlen(search), ':');
    if (!start) return 0;

    return atoi(start + 1);
}

// 파일 읽기
void serve_file(int client_fd, const char *filepath) {
    FILE *file = fopen(filepath, "r");
    if (!file) {
        const char *err = "404 Not Found";
        send_http_response(client_fd, 404, "text/plain", err);
        return;
    }

    fseek(file, 0, SEEK_END);
    long fsize = ftell(file);
    fseek(file, 0, SEEK_SET);

    char *content = malloc(fsize + 1);
    fread(content, 1, fsize, file);
    fclose(file);
    content[fsize] = 0;

    const char *content_type = "text/html; charset=utf-8";
    if (strstr(filepath, ".js")) content_type = "application/javascript";
    else if (strstr(filepath, ".css")) content_type = "text/css";

    send_http_response(client_fd, 200, content_type, content);
    free(content);
}

// SQLite에서 최신 데이터 조회
void get_latest_stats(char *json_out, size_t max_size) {
    sqlite3 *db;
    sqlite3_stmt *stmt;
    int rc = sqlite3_open(DB_PATH, &db);

    if (rc != SQLITE_OK) {
        snprintf(json_out, max_size, "{\"success\":false,\"error\":\"DB open failed\"}");
        return;
    }

    const char *sql = "SELECT fitness, generation, workers, status FROM evolution_stats "
                      "ORDER BY timestamp DESC LIMIT 1";

    rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        snprintf(json_out, max_size, "{\"success\":false,\"error\":\"Query failed\"}");
        sqlite3_close(db);
        return;
    }

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        snprintf(json_out, max_size,
            "{\"success\":true,\"latest\":{\"fitness\":%.6f,\"generation\":%d,\"workers\":%d,\"status\":\"%s\"}}",
            sqlite3_column_double(stmt, 0),
            sqlite3_column_int(stmt, 1),
            sqlite3_column_int(stmt, 2),
            sqlite3_column_text(stmt, 3));
    } else {
        snprintf(json_out, max_size, "{\"success\":false,\"error\":\"No data\"}");
    }

    sqlite3_finalize(stmt);
    sqlite3_close(db);
}

// 히스토리 데이터 조회
void get_history_stats(char *json_out, size_t max_size) {
    sqlite3 *db;
    sqlite3_stmt *stmt;
    int rc = sqlite3_open(DB_PATH, &db);

    if (rc != SQLITE_OK) {
        snprintf(json_out, max_size, "[]");
        return;
    }

    const char *sql = "SELECT timestamp, fitness, generation, workers, status "
                      "FROM evolution_stats ORDER BY timestamp DESC LIMIT 50";

    rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        snprintf(json_out, max_size, "[]");
        sqlite3_close(db);
        return;
    }

    strcpy(json_out, "[");
    int first = 1;

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        char item[256];
        snprintf(item, sizeof(item),
            "%s{\"timestamp\":%ld,\"fitness\":%.6f,\"generation\":%d,\"workers\":%d,\"status\":\"%s\"}",
            first ? "" : ",",
            sqlite3_column_int64(stmt, 0),
            sqlite3_column_double(stmt, 1),
            sqlite3_column_int(stmt, 2),
            sqlite3_column_int(stmt, 3),
            sqlite3_column_text(stmt, 4));

        strcat(json_out, item);
        first = 0;
    }

    strcat(json_out, "]");

    sqlite3_finalize(stmt);
    sqlite3_close(db);
}

// Gateway 최신 데이터 조회
void get_latest_gateway(char *json_out, size_t max_size) {
    sqlite3 *db;
    sqlite3_stmt *stmt;
    int rc = sqlite3_open(DB_PATH, &db);

    if (rc != SQLITE_OK) {
        snprintf(json_out, max_size, "null");
        return;
    }

    const char *sql = "SELECT server, pid, phase, status, uptime_days, "
                      "total_commands, success_rate, block_rate, memory_usage_mb, level "
                      "FROM gateway_stats ORDER BY timestamp DESC LIMIT 1";

    rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        snprintf(json_out, max_size, "null");
        sqlite3_close(db);
        return;
    }

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        snprintf(json_out, max_size,
            "{\"server\":\"%s\",\"pid\":%d,\"phase\":\"%s\",\"status\":\"%s\","
            "\"uptime_days\":%.2f,\"total_commands\":%d,\"success_rate\":%.1f,"
            "\"block_rate\":%.1f,\"memory_usage_mb\":%d,\"level\":\"%s\"}",
            sqlite3_column_text(stmt, 0),
            sqlite3_column_int(stmt, 1),
            sqlite3_column_text(stmt, 2),
            sqlite3_column_text(stmt, 3),
            sqlite3_column_double(stmt, 4),
            sqlite3_column_int(stmt, 5),
            sqlite3_column_double(stmt, 6),
            sqlite3_column_double(stmt, 7),
            sqlite3_column_int(stmt, 8),
            sqlite3_column_text(stmt, 9));
    } else {
        snprintf(json_out, max_size, "null");
    }

    sqlite3_finalize(stmt);
    sqlite3_close(db);
}

// 에러 로그 조회
void get_errors(char *json_out, size_t max_size) {
    sqlite3 *db;
    sqlite3_stmt *stmt;
    int rc = sqlite3_open(DB_PATH, &db);

    if (rc != SQLITE_OK) {
        snprintf(json_out, max_size, "[]");
        return;
    }

    const char *sql = "SELECT timestamp, error_msg FROM error_logs "
                      "ORDER BY timestamp DESC LIMIT 10";

    rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        snprintf(json_out, max_size, "[]");
        sqlite3_close(db);
        return;
    }

    strcpy(json_out, "[");
    int first = 1;

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        char item[512];
        snprintf(item, sizeof(item),
            "%s{\"timestamp\":%ld,\"error_msg\":\"%s\"}",
            first ? "" : ",",
            sqlite3_column_int64(stmt, 0),
            sqlite3_column_text(stmt, 1));

        strcat(json_out, item);
        first = 0;
    }

    strcat(json_out, "]");

    sqlite3_finalize(stmt);
    sqlite3_close(db);
}

// POST body 추출
const char* extract_post_body(const char *request) {
    const char *body = strstr(request, "\r\n\r\n");
    if (body) return body + 4;
    return NULL;
}

// API 라우팅
void handle_request(int client_fd, const char *request) {
    char response[BUFFER_SIZE] = {0};

    // GET /
    if (strstr(request, "GET / ") || strstr(request, "GET /index.html ")) {
        serve_file(client_fd, "../web/index.html");
    }
    // GET /dashboard.html
    else if (strstr(request, "GET /dashboard.html")) {
        serve_file(client_fd, "../web/dashboard.html");
    }
    // GET /js/*.js
    else if (strstr(request, "GET /js/")) {
        const char *path = strstr(request, "/js/");
        char *end = strchr(path, ' ');
        if (end) {
            char filename[256];
            int len = end - path;
            if (len > 0 && len < 255) {
                strncpy(filename, path, len);
                filename[len] = '\0';

                char filepath[512];
                snprintf(filepath, sizeof(filepath), "../web%s", filename);
                serve_file(client_fd, filepath);
                return;
            }
        }
        send_http_response(client_fd, 404, "text/plain", "404 Not Found");
    }
    // GET /css/*.css
    else if (strstr(request, "GET /css/")) {
        const char *path = strstr(request, "/css/");
        char *end = strchr(path, ' ');
        if (end) {
            char filename[256];
            int len = end - path;
            if (len > 0 && len < 255) {
                strncpy(filename, path, len);
                filename[len] = '\0';

                char filepath[512];
                snprintf(filepath, sizeof(filepath), "../web%s", filename);
                serve_file(client_fd, filepath);
                return;
            }
        }
        send_http_response(client_fd, 404, "text/plain", "404 Not Found");
    }
    // GET /api/stats (레거시 호환)
    else if (strstr(request, "GET /api/stats ")) {
        char latest[1024] = {0};
        char history[32768] = {0};
        char errors[8192] = {0};
        char gateway[1024] = {0};

        get_latest_stats(latest, sizeof(latest));
        get_history_stats(history, sizeof(history));
        get_errors(errors, sizeof(errors));
        get_latest_gateway(gateway, sizeof(gateway));

        snprintf(response, sizeof(response),
            "{\"success\":true,\"latest\":%s,\"history\":%s,\"errors\":%s,\"gateway\":%s}",
            strchr(latest, '{') ? strchr(latest, '{') : "null",
            history,
            errors,
            gateway);

        send_http_response(client_fd, 200, "application/json", response);
    }
    // GET /api/dashboards/{id}
    else if (strstr(request, "GET /api/dashboards/")) {
        const char *path = strstr(request, "/api/dashboards/");
        int id = atoi(path + 16);

        if (id > 0) {
            dashboard_get(DB_PATH, id, response, sizeof(response));
            send_http_response(client_fd, 200, "application/json", response);
        } else {
            send_http_response(client_fd, 404, "application/json",
                "{\"success\":false,\"error\":\"Invalid ID\"}");
        }
    }
    // GET /api/dashboards (must come after /api/dashboards/{id})
    else if (strstr(request, "GET /api/dashboards")) {
        dashboard_list(DB_PATH, response, sizeof(response));
        send_http_response(client_fd, 200, "application/json", response);
    }
    // POST /api/dashboards/{id}/widgets
    else if (strstr(request, "POST /api/dashboards/") && strstr(request, "/widgets")) {
        const char *path = strstr(request, "/api/dashboards/");
        int dashboard_id = atoi(path + 16);

        const char *body = extract_post_body(request);
        if (!body) {
            send_http_response(client_fd, 400, "application/json",
                "{\"success\":false,\"error\":\"No body\"}");
            return;
        }

        char widget_type[32], title[128], data_source[64], query[512], config[512];
        extract_json_string(body, "widget_type", widget_type, sizeof(widget_type));
        extract_json_string(body, "title", title, sizeof(title));
        extract_json_string(body, "data_source", data_source, sizeof(data_source));
        extract_json_string(body, "query", query, sizeof(query));
        extract_json_string(body, "config", config, sizeof(config));

        int x = extract_json_int(body, "position_x");
        int y = extract_json_int(body, "position_y");
        int w = extract_json_int(body, "width");
        int h = extract_json_int(body, "height");

        if (!config[0]) strcpy(config, "{}");

        int widget_id = widget_create(DB_PATH, dashboard_id, widget_type, title,
                                       data_source, query, config, x, y, w, h);

        if (widget_id > 0) {
            snprintf(response, sizeof(response),
                "{\"success\":true,\"widget_id\":%d}", widget_id);
            send_http_response(client_fd, 200, "application/json", response);
        } else if (widget_id == -2) {
            send_http_response(client_fd, 400, "application/json",
                "{\"success\":false,\"error\":\"Unsafe query\"}");
        } else {
            send_http_response(client_fd, 500, "application/json",
                "{\"success\":false,\"error\":\"Creation failed\"}");
        }
    }
    // POST /api/dashboards/{id}/set-default
    else if (strstr(request, "POST /api/dashboards/") && strstr(request, "/set-default")) {
        const char *path = strstr(request, "/api/dashboards/");
        int id = atoi(path + 16);

        if (dashboard_set_default(DB_PATH, id) == 0) {
            send_http_response(client_fd, 200, "application/json",
                "{\"success\":true}");
        } else {
            send_http_response(client_fd, 500, "application/json",
                "{\"success\":false,\"error\":\"Update failed\"}");
        }
    }
    // POST /api/dashboards
    else if (strstr(request, "POST /api/dashboards")) {
        const char *body = extract_post_body(request);
        if (!body) {
            send_http_response(client_fd, 400, "application/json",
                "{\"success\":false,\"error\":\"No body\"}");
            return;
        }

        char name[64], title[128], desc[256], layout[16];
        extract_json_string(body, "name", name, sizeof(name));
        extract_json_string(body, "title", title, sizeof(title));
        extract_json_string(body, "description", desc, sizeof(desc));
        extract_json_string(body, "layout", layout, sizeof(layout));

        if (!layout[0]) strcpy(layout, "grid");

        int id = dashboard_create(DB_PATH, name, title, desc, layout);
        if (id > 0) {
            snprintf(response, sizeof(response),
                "{\"success\":true,\"dashboard_id\":%d,\"name\":\"%s\"}", id, name);
            send_http_response(client_fd, 200, "application/json", response);
        } else {
            send_http_response(client_fd, 500, "application/json",
                "{\"success\":false,\"error\":\"Creation failed\"}");
        }
    }
    // GET /api/dashboards/{id}
    else if (strstr(request, "GET /api/dashboards/")) {
        const char *path = strstr(request, "/api/dashboards/");
        int id = atoi(path + 16);

        if (id > 0) {
            dashboard_get(DB_PATH, id, response, sizeof(response));
            send_http_response(client_fd, 200, "application/json", response);
        } else {
            send_http_response(client_fd, 404, "application/json",
                "{\"success\":false,\"error\":\"Invalid ID\"}");
        }
    }
    // DELETE /api/dashboards/{id}
    else if (strstr(request, "DELETE /api/dashboards/")) {
        const char *path = strstr(request, "/api/dashboards/");
        int id = atoi(path + 16);

        if (dashboard_delete(DB_PATH, id) == 0) {
            send_http_response(client_fd, 200, "application/json",
                "{\"success\":true}");
        } else {
            send_http_response(client_fd, 500, "application/json",
                "{\"success\":false,\"error\":\"Deletion failed\"}");
        }
    }
    // POST /api/dashboards/{id}/set-default
    else if (strstr(request, "POST /api/dashboards/") && strstr(request, "/set-default")) {
        const char *path = strstr(request, "/api/dashboards/");
        int id = atoi(path + 16);

        if (dashboard_set_default(DB_PATH, id) == 0) {
            send_http_response(client_fd, 200, "application/json",
                "{\"success\":true}");
        } else {
            send_http_response(client_fd, 500, "application/json",
                "{\"success\":false,\"error\":\"Update failed\"}");
        }
    }
    // POST /api/dashboards/{id}/widgets
    else if (strstr(request, "POST /api/dashboards/") && strstr(request, "/widgets")) {
        const char *path = strstr(request, "/api/dashboards/");
        int dashboard_id = atoi(path + 16);

        const char *body = extract_post_body(request);
        if (!body) {
            send_http_response(client_fd, 400, "application/json",
                "{\"success\":false,\"error\":\"No body\"}");
            return;
        }

        char widget_type[32], title[128], data_source[64], query[512], config[512];
        extract_json_string(body, "widget_type", widget_type, sizeof(widget_type));
        extract_json_string(body, "title", title, sizeof(title));
        extract_json_string(body, "data_source", data_source, sizeof(data_source));
        extract_json_string(body, "query", query, sizeof(query));
        extract_json_string(body, "config", config, sizeof(config));

        int x = extract_json_int(body, "position_x");
        int y = extract_json_int(body, "position_y");
        int w = extract_json_int(body, "width");
        int h = extract_json_int(body, "height");

        if (!config[0]) strcpy(config, "{}");

        int widget_id = widget_create(DB_PATH, dashboard_id, widget_type, title,
                                       data_source, query, config, x, y, w, h);

        if (widget_id > 0) {
            snprintf(response, sizeof(response),
                "{\"success\":true,\"widget_id\":%d}", widget_id);
            send_http_response(client_fd, 200, "application/json", response);
        } else if (widget_id == -2) {
            send_http_response(client_fd, 400, "application/json",
                "{\"success\":false,\"error\":\"Unsafe query\"}");
        } else {
            send_http_response(client_fd, 500, "application/json",
                "{\"success\":false,\"error\":\"Creation failed\"}");
        }
    }
    // DELETE /api/widgets/{id}
    else if (strstr(request, "DELETE /api/widgets/")) {
        const char *path = strstr(request, "/api/widgets/");
        int id = atoi(path + 13);

        if (widget_delete(DB_PATH, id) == 0) {
            send_http_response(client_fd, 200, "application/json",
                "{\"success\":true}");
        } else {
            send_http_response(client_fd, 500, "application/json",
                "{\"success\":false,\"error\":\"Deletion failed\"}");
        }
    }
    // GET /api/widgets/{id}/data
    else if (strstr(request, "GET /api/widgets/") && strstr(request, "/data")) {
        const char *path = strstr(request, "/api/widgets/");
        int id = atoi(path + 13);

        widget_get_data(DB_PATH, id, response, sizeof(response));
        send_http_response(client_fd, 200, "application/json", response);
    }
    // GET /api/chart/line?widget_id={id}
    else if (strstr(request, "GET /api/chart/line")) {
        // Extract widget_id from query string
        const char *query = strstr(request, "widget_id=");
        if (!query) {
            send_http_response(client_fd, 400, "text/plain", "Missing widget_id");
            return;
        }

        int widget_id = atoi(query + 10);

        // Get widget data
        char data_json[BUFFER_SIZE] = {0};
        widget_get_data(DB_PATH, widget_id, data_json, sizeof(data_json));

        // Parse JSON data to extract values and labels
        // Simple parsing for array of {timestamp, value}
        double values[50];
        char *labels[50];
        int count = 0;

        const char *ptr = strstr(data_json, "[");
        if (ptr) {
            ptr++;
            while (*ptr && count < 50) {
                // Skip whitespace and commas
                while (*ptr == ' ' || *ptr == ',' || *ptr == '\n') ptr++;
                if (*ptr != '{') break;

                // Extract timestamp (label)
                const char *ts_start = strstr(ptr, "\"timestamp\":");
                if (ts_start) {
                    ts_start += 12;
                    long timestamp = atol(ts_start);

                    // Convert timestamp to simple label
                    labels[count] = malloc(16);
                    snprintf(labels[count], 16, "%ld", timestamp % 10000);
                }

                // Extract value (fitness, generation, etc - first numeric field after timestamp)
                const char *val_start = strchr(ts_start, ':');
                if (val_start) {
                    val_start = strchr(val_start + 1, ':');
                    if (val_start) {
                        values[count] = atof(val_start + 1);
                    }
                }

                count++;
                ptr = strchr(ptr, '}');
                if (ptr) ptr++;
            }
        }

        if (count == 0) {
            send_http_response(client_fd, 500, "text/plain", "No data to chart");
            return;
        }

        // Create chart data
        ChartData chart_data = {
            .values = values,
            .labels = labels,
            .count = count,
            .title = "Fitness Evolution",
            .color = "#58a6ff"
        };

        // Create chart options
        ChartOptions opts = chart_default_options();
        opts.width = 800;
        opts.height = 400;

        // Generate SVG
        char *svg = chart_create(CHART_LINE, &chart_data, &opts);

        // Free labels
        for (int i = 0; i < count; i++) {
            free(labels[i]);
        }

        if (svg) {
            send_http_response(client_fd, 200, "image/svg+xml", svg);
            chart_free(svg);
        } else {
            send_http_response(client_fd, 500, "text/plain", "Chart generation failed");
        }
    }
    else {
        send_http_response(client_fd, 404, "text/plain", "404 Not Found");
    }
}

int main() {
    int server_fd, client_fd;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    char buffer[BUFFER_SIZE] = {0};

    // 데이터베이스 스키마 초기화
    if (dashboard_init_schema(DB_PATH) != 0) {
        fprintf(stderr, "Failed to initialize dashboard schema\n");
    }

    // 소켓 생성
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == 0) {
        perror("Socket failed");
        exit(EXIT_FAILURE);
    }

    // 재사용 옵션
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // 바인딩
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    // 리스닝
    if (listen(server_fd, 10) < 0) {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    printf("[AEON Multi-Dashboard API Server] 시작됨 - http://localhost:%d\n", PORT);
    printf("[철학] 0%% 외부 의존성 | 순수 C 구현\n");
    printf("[구조] SQLite → C HTTP Server → Pure HTML/JS\n");
    printf("[기능] Multi Dashboard + Dynamic Widgets\n\n");

    // 요청 처리 루프
    while (1) {
        client_fd = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen);
        if (client_fd < 0) {
            perror("Accept failed");
            continue;
        }

        memset(buffer, 0, BUFFER_SIZE);
        read(client_fd, buffer, BUFFER_SIZE);

        printf("[요청] %.*s\n", 50, buffer);
        handle_request(client_fd, buffer);

        close(client_fd);
    }

    close(server_fd);
    return 0;
}
