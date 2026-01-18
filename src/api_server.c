#include "aeon_monitor.h"
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
void send_http_response(int client_fd, const char *content_type, const char *body) {
    char header[1024];
    snprintf(header, sizeof(header),
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: %s\r\n"
        "Access-Control-Allow-Origin: *\r\n"
        "Content-Length: %ld\r\n"
        "Connection: close\r\n"
        "\r\n",
        content_type, strlen(body));

    write(client_fd, header, strlen(header));
    write(client_fd, body, strlen(body));
}

// 파일 읽기
void serve_file(int client_fd, const char *filepath) {
    FILE *file = fopen(filepath, "r");
    if (!file) {
        const char *err = "HTTP/1.1 404 Not Found\r\n\r\n404 Not Found";
        write(client_fd, err, strlen(err));
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
    send_http_response(client_fd, content_type, content);
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

// 히스토리 데이터 조회 (최근 50개)
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

// API 라우팅
void handle_request(int client_fd, const char *request) {
    if (strstr(request, "GET / ") || strstr(request, "GET /index.html ")) {
        serve_file(client_fd, "../web/dashboard.html");
    }
    else if (strstr(request, "GET /api/stats ")) {
        char latest[1024] = {0};
        char history[32768] = {0};
        char errors[8192] = {0};
        char response[BUFFER_SIZE] = {0};

        get_latest_stats(latest, sizeof(latest));
        get_history_stats(history, sizeof(history));
        get_errors(errors, sizeof(errors));

        // JSON 병합
        snprintf(response, sizeof(response),
            "{\"success\":true,\"latest\":%s,\"history\":%s,\"errors\":%s}",
            strchr(latest, '{') ? strchr(latest, '{') : "null",
            history,
            errors);

        send_http_response(client_fd, "application/json", response);
    }
    else {
        const char *err = "HTTP/1.1 404 Not Found\r\n\r\n404 Not Found";
        write(client_fd, err, strlen(err));
    }
}

int main() {
    int server_fd, client_fd;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    char buffer[BUFFER_SIZE] = {0};

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

    printf("[AEON API Server] 시작됨 - http://localhost:%d\n", PORT);
    printf("[철학] 0%% 외부 의존성 | 순수 C 구현\n");
    printf("[구조] SQLite → C HTTP Server → Pure HTML/JS\n\n");

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
