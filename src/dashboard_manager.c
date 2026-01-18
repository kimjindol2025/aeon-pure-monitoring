#include "dashboard_manager.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <time.h>
#include <ctype.h>

// 허용된 테이블 (보안)
static const char *allowed_tables[] = {
    "evolution_stats",
    "gateway_stats",
    "error_logs",
    "system_metrics",
    "api_requests",
    "worker_status",
    NULL
};

// 쿼리 안전성 검증
int is_query_safe(const char *query) {
    if (!query) return 0;

    // SELECT만 허용
    if (strncasecmp(query, "SELECT", 6) != 0) {
        return 0;
    }

    // 위험한 키워드 검사
    const char *forbidden[] = {"DROP", "DELETE", "UPDATE", "INSERT", "ALTER", "CREATE", NULL};
    for (int i = 0; forbidden[i]; i++) {
        if (strcasestr(query, forbidden[i])) {
            return 0;
        }
    }

    // 허용 테이블 검증
    int found = 0;
    for (int i = 0; allowed_tables[i]; i++) {
        if (strstr(query, allowed_tables[i])) {
            found = 1;
            break;
        }
    }

    return found;
}

// 스키마 초기화
int dashboard_init_schema(const char *db_path) {
    sqlite3 *db;
    char *err_msg = NULL;

    int rc = sqlite3_open(db_path, &db);
    if (rc != SQLITE_OK) {
        return -1;
    }

    // dashboards 테이블
    const char *sql_dashboards =
        "CREATE TABLE IF NOT EXISTS dashboards ("
        "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  name TEXT NOT NULL UNIQUE,"
        "  title TEXT NOT NULL,"
        "  description TEXT,"
        "  layout TEXT DEFAULT 'grid',"
        "  created_at INTEGER NOT NULL,"
        "  updated_at INTEGER NOT NULL,"
        "  is_default INTEGER DEFAULT 0"
        ");";

    rc = sqlite3_exec(db, sql_dashboards, 0, 0, &err_msg);
    if (rc != SQLITE_OK) {
        sqlite3_free(err_msg);
        sqlite3_close(db);
        return -1;
    }

    // widgets 테이블
    const char *sql_widgets =
        "CREATE TABLE IF NOT EXISTS widgets ("
        "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  dashboard_id INTEGER NOT NULL,"
        "  widget_type TEXT NOT NULL,"
        "  title TEXT NOT NULL,"
        "  data_source TEXT NOT NULL,"
        "  query TEXT,"
        "  config TEXT,"
        "  position_x INTEGER DEFAULT 0,"
        "  position_y INTEGER DEFAULT 0,"
        "  width INTEGER DEFAULT 4,"
        "  height INTEGER DEFAULT 3,"
        "  created_at INTEGER NOT NULL,"
        "  FOREIGN KEY (dashboard_id) REFERENCES dashboards(id) ON DELETE CASCADE"
        ");";

    rc = sqlite3_exec(db, sql_widgets, 0, 0, &err_msg);
    if (rc != SQLITE_OK) {
        sqlite3_free(err_msg);
        sqlite3_close(db);
        return -1;
    }

    sqlite3_close(db);
    return 0;
}

// 대시보드 생성
int dashboard_create(const char *db_path, const char *name, const char *title,
                     const char *description, const char *layout) {
    sqlite3 *db;
    sqlite3_stmt *stmt;

    int rc = sqlite3_open(db_path, &db);
    if (rc != SQLITE_OK) return -1;

    long now = time(NULL);
    const char *sql = "INSERT INTO dashboards (name, title, description, layout, created_at, updated_at) "
                      "VALUES (?, ?, ?, ?, ?, ?)";

    rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        sqlite3_close(db);
        return -1;
    }

    sqlite3_bind_text(stmt, 1, name, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, title, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, description, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 4, layout, -1, SQLITE_STATIC);
    sqlite3_bind_int64(stmt, 5, now);
    sqlite3_bind_int64(stmt, 6, now);

    rc = sqlite3_step(stmt);
    int dashboard_id = -1;

    if (rc == SQLITE_DONE) {
        dashboard_id = sqlite3_last_insert_rowid(db);
    }

    sqlite3_finalize(stmt);
    sqlite3_close(db);

    return dashboard_id;
}

// 대시보드 목록 조회
int dashboard_list(const char *db_path, char *json_out, size_t max_size) {
    sqlite3 *db;
    sqlite3_stmt *stmt;

    int rc = sqlite3_open(db_path, &db);
    if (rc != SQLITE_OK) {
        snprintf(json_out, max_size, "{\"success\":false,\"error\":\"DB open failed\"}");
        return -1;
    }

    const char *sql = "SELECT d.id, d.name, d.title, d.description, d.layout, d.is_default, "
                      "d.created_at, COUNT(w.id) as widget_count "
                      "FROM dashboards d LEFT JOIN widgets w ON d.id = w.dashboard_id "
                      "GROUP BY d.id ORDER BY d.is_default DESC, d.created_at ASC";

    rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        snprintf(json_out, max_size, "{\"success\":false,\"error\":\"Query failed\"}");
        sqlite3_close(db);
        return -1;
    }

    strcpy(json_out, "{\"success\":true,\"dashboards\":[");
    int first = 1;

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        char item[512];
        snprintf(item, sizeof(item),
            "%s{\"id\":%d,\"name\":\"%s\",\"title\":\"%s\",\"description\":\"%s\","
            "\"layout\":\"%s\",\"widget_count\":%d,\"is_default\":%d,\"created_at\":%ld}",
            first ? "" : ",",
            sqlite3_column_int(stmt, 0),
            sqlite3_column_text(stmt, 1),
            sqlite3_column_text(stmt, 2),
            sqlite3_column_text(stmt, 3) ? sqlite3_column_text(stmt, 3) : "",
            sqlite3_column_text(stmt, 4),
            sqlite3_column_int(stmt, 7),
            sqlite3_column_int(stmt, 5),
            sqlite3_column_int64(stmt, 6));

        strcat(json_out, item);
        first = 0;
    }

    strcat(json_out, "]}");

    sqlite3_finalize(stmt);
    sqlite3_close(db);
    return 0;
}

// 대시보드 조회 (위젯 포함)
int dashboard_get(const char *db_path, int id, char *json_out, size_t max_size) {
    sqlite3 *db;
    sqlite3_stmt *stmt;

    int rc = sqlite3_open(db_path, &db);
    if (rc != SQLITE_OK) {
        snprintf(json_out, max_size, "{\"success\":false,\"error\":\"DB open failed\"}");
        return -1;
    }

    // 대시보드 정보
    const char *sql = "SELECT id, name, title, description, layout, created_at, updated_at "
                      "FROM dashboards WHERE id = ?";

    rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        sqlite3_close(db);
        return -1;
    }

    sqlite3_bind_int(stmt, 1, id);

    if (sqlite3_step(stmt) != SQLITE_ROW) {
        snprintf(json_out, max_size, "{\"success\":false,\"error\":\"Dashboard not found\"}");
        sqlite3_finalize(stmt);
        sqlite3_close(db);
        return -1;
    }

    snprintf(json_out, max_size,
        "{\"success\":true,\"dashboard\":{\"id\":%d,\"name\":\"%s\",\"title\":\"%s\","
        "\"description\":\"%s\",\"layout\":\"%s\",\"created_at\":%ld,\"updated_at\":%ld},\"widgets\":[",
        sqlite3_column_int(stmt, 0),
        sqlite3_column_text(stmt, 1),
        sqlite3_column_text(stmt, 2),
        sqlite3_column_text(stmt, 3) ? sqlite3_column_text(stmt, 3) : "",
        sqlite3_column_text(stmt, 4),
        sqlite3_column_int64(stmt, 5),
        sqlite3_column_int64(stmt, 6));

    sqlite3_finalize(stmt);

    // 위젯 목록
    char widgets_json[32768];
    widget_list(db_path, id, widgets_json, sizeof(widgets_json));

    // widgets_json에서 배열 부분만 추출
    char *array_start = strchr(widgets_json, '[');
    if (array_start) {
        char *array_end = strrchr(widgets_json, ']');
        if (array_end) {
            *array_end = '\0';
            strcat(json_out, array_start + 1);
        }
    }

    strcat(json_out, "]}");

    sqlite3_close(db);
    return 0;
}

// 대시보드 삭제
int dashboard_delete(const char *db_path, int id) {
    sqlite3 *db;
    sqlite3_stmt *stmt;

    int rc = sqlite3_open(db_path, &db);
    if (rc != SQLITE_OK) return -1;

    const char *sql = "DELETE FROM dashboards WHERE id = ?";

    rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        sqlite3_close(db);
        return -1;
    }

    sqlite3_bind_int(stmt, 1, id);
    rc = sqlite3_step(stmt);

    sqlite3_finalize(stmt);
    sqlite3_close(db);

    return (rc == SQLITE_DONE) ? 0 : -1;
}

// 기본 대시보드 설정
int dashboard_set_default(const char *db_path, int id) {
    sqlite3 *db;
    char *err_msg = NULL;

    int rc = sqlite3_open(db_path, &db);
    if (rc != SQLITE_OK) return -1;

    // 모든 대시보드 is_default = 0
    rc = sqlite3_exec(db, "UPDATE dashboards SET is_default = 0", 0, 0, &err_msg);
    if (rc != SQLITE_OK) {
        sqlite3_free(err_msg);
        sqlite3_close(db);
        return -1;
    }

    // 지정된 대시보드만 is_default = 1
    char sql[128];
    snprintf(sql, sizeof(sql), "UPDATE dashboards SET is_default = 1 WHERE id = %d", id);
    rc = sqlite3_exec(db, sql, 0, 0, &err_msg);

    if (rc != SQLITE_OK) {
        sqlite3_free(err_msg);
        sqlite3_close(db);
        return -1;
    }

    sqlite3_close(db);
    return 0;
}

// 위젯 생성
int widget_create(const char *db_path, int dashboard_id, const char *widget_type,
                  const char *title, const char *data_source, const char *query,
                  const char *config, int x, int y, int w, int h) {
    // 쿼리 안전성 검증
    if (query && !is_query_safe(query)) {
        return -2; // 안전하지 않은 쿼리
    }

    sqlite3 *db;
    sqlite3_stmt *stmt;

    int rc = sqlite3_open(db_path, &db);
    if (rc != SQLITE_OK) return -1;

    long now = time(NULL);
    const char *sql = "INSERT INTO widgets (dashboard_id, widget_type, title, data_source, "
                      "query, config, position_x, position_y, width, height, created_at) "
                      "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";

    rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        sqlite3_close(db);
        return -1;
    }

    sqlite3_bind_int(stmt, 1, dashboard_id);
    sqlite3_bind_text(stmt, 2, widget_type, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, title, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 4, data_source, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 5, query, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 6, config, -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 7, x);
    sqlite3_bind_int(stmt, 8, y);
    sqlite3_bind_int(stmt, 9, w);
    sqlite3_bind_int(stmt, 10, h);
    sqlite3_bind_int64(stmt, 11, now);

    rc = sqlite3_step(stmt);
    int widget_id = -1;

    if (rc == SQLITE_DONE) {
        widget_id = sqlite3_last_insert_rowid(db);
    }

    sqlite3_finalize(stmt);
    sqlite3_close(db);

    return widget_id;
}

// 위젯 목록 조회
int widget_list(const char *db_path, int dashboard_id, char *json_out, size_t max_size) {
    sqlite3 *db;
    sqlite3_stmt *stmt;

    int rc = sqlite3_open(db_path, &db);
    if (rc != SQLITE_OK) {
        snprintf(json_out, max_size, "{\"success\":false,\"error\":\"DB open failed\"}");
        return -1;
    }

    const char *sql = "SELECT id, widget_type, title, data_source, query, config, "
                      "position_x, position_y, width, height "
                      "FROM widgets WHERE dashboard_id = ? ORDER BY position_y, position_x";

    rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        sqlite3_close(db);
        return -1;
    }

    sqlite3_bind_int(stmt, 1, dashboard_id);

    strcpy(json_out, "{\"success\":true,\"widgets\":[");
    int first = 1;

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        char item[1024];
        const char *query_text = (const char*)sqlite3_column_text(stmt, 4);
        const char *config_text = (const char*)sqlite3_column_text(stmt, 5);

        snprintf(item, sizeof(item),
            "%s{\"id\":%d,\"widget_type\":\"%s\",\"title\":\"%s\",\"data_source\":\"%s\","
            "\"query\":\"%s\",\"config\":%s,\"position_x\":%d,\"position_y\":%d,"
            "\"width\":%d,\"height\":%d}",
            first ? "" : ",",
            sqlite3_column_int(stmt, 0),
            (const char*)sqlite3_column_text(stmt, 1),
            (const char*)sqlite3_column_text(stmt, 2),
            (const char*)sqlite3_column_text(stmt, 3),
            query_text ? query_text : "",
            config_text ? config_text : "{}",
            sqlite3_column_int(stmt, 6),
            sqlite3_column_int(stmt, 7),
            sqlite3_column_int(stmt, 8),
            sqlite3_column_int(stmt, 9));

        strcat(json_out, item);
        first = 0;
    }

    strcat(json_out, "]}");

    sqlite3_finalize(stmt);
    sqlite3_close(db);
    return 0;
}

// 위젯 삭제
int widget_delete(const char *db_path, int widget_id) {
    sqlite3 *db;
    sqlite3_stmt *stmt;

    int rc = sqlite3_open(db_path, &db);
    if (rc != SQLITE_OK) return -1;

    const char *sql = "DELETE FROM widgets WHERE id = ?";

    rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        sqlite3_close(db);
        return -1;
    }

    sqlite3_bind_int(stmt, 1, widget_id);
    rc = sqlite3_step(stmt);

    sqlite3_finalize(stmt);
    sqlite3_close(db);

    return (rc == SQLITE_DONE) ? 0 : -1;
}

// 위젯 데이터 조회
int widget_get_data(const char *db_path, int widget_id, char *json_out, size_t max_size) {
    sqlite3 *db;
    sqlite3_stmt *stmt;

    int rc = sqlite3_open(db_path, &db);
    if (rc != SQLITE_OK) {
        snprintf(json_out, max_size, "{\"success\":false,\"error\":\"DB open failed\"}");
        return -1;
    }

    // 위젯의 쿼리 가져오기
    const char *sql = "SELECT query FROM widgets WHERE id = ?";
    rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        sqlite3_close(db);
        return -1;
    }

    sqlite3_bind_int(stmt, 1, widget_id);

    if (sqlite3_step(stmt) != SQLITE_ROW) {
        snprintf(json_out, max_size, "{\"success\":false,\"error\":\"Widget not found\"}");
        sqlite3_finalize(stmt);
        sqlite3_close(db);
        return -1;
    }

    const char *query = (const char *)sqlite3_column_text(stmt, 0);
    if (!query || !is_query_safe(query)) {
        snprintf(json_out, max_size, "{\"success\":false,\"error\":\"Invalid query\"}");
        sqlite3_finalize(stmt);
        sqlite3_close(db);
        return -1;
    }

    char query_copy[512];
    strncpy(query_copy, query, sizeof(query_copy) - 1);
    sqlite3_finalize(stmt);

    // 쿼리 실행
    rc = sqlite3_prepare_v2(db, query_copy, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        snprintf(json_out, max_size, "{\"success\":false,\"error\":\"Query execution failed\"}");
        sqlite3_close(db);
        return -1;
    }

    strcpy(json_out, "{\"success\":true,\"data\":[");
    int first = 1;
    int col_count = sqlite3_column_count(stmt);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        char row[512] = "{";
        for (int i = 0; i < col_count; i++) {
            char field[128];
            const char *col_name = sqlite3_column_name(stmt, i);
            int col_type = sqlite3_column_type(stmt, i);

            if (col_type == SQLITE_INTEGER) {
                snprintf(field, sizeof(field), "%s\"%s\":%lld",
                    i > 0 ? "," : "",
                    col_name,
                    sqlite3_column_int64(stmt, i));
            } else if (col_type == SQLITE_FLOAT) {
                snprintf(field, sizeof(field), "%s\"%s\":%.6f",
                    i > 0 ? "," : "",
                    col_name,
                    sqlite3_column_double(stmt, i));
            } else {
                snprintf(field, sizeof(field), "%s\"%s\":\"%s\"",
                    i > 0 ? "," : "",
                    col_name,
                    sqlite3_column_text(stmt, i) ? sqlite3_column_text(stmt, i) : "");
            }
            strcat(row, field);
        }
        strcat(row, "}");

        if (!first) strcat(json_out, ",");
        strcat(json_out, row);
        first = 0;
    }

    strcat(json_out, "]}");

    sqlite3_finalize(stmt);
    sqlite3_close(db);
    return 0;
}
