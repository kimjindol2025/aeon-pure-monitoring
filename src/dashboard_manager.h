#ifndef DASHBOARD_MANAGER_H
#define DASHBOARD_MANAGER_H

#include <stddef.h>
#include <sqlite3.h>

// 대시보드 구조체
typedef struct {
    int id;
    char name[64];
    char title[128];
    char description[256];
    char layout[16];
    long created_at;
    long updated_at;
    int is_default;
} Dashboard;

// 위젯 구조체
typedef struct {
    int id;
    int dashboard_id;
    char widget_type[32];
    char title[128];
    char data_source[64];
    char query[512];
    char config[512];
    int position_x;
    int position_y;
    int width;
    int height;
    long created_at;
} Widget;

// 대시보드 관리 함수
int dashboard_init_schema(const char *db_path);
int dashboard_create(const char *db_path, const char *name, const char *title,
                     const char *description, const char *layout);
int dashboard_list(const char *db_path, char *json_out, size_t max_size);
int dashboard_get(const char *db_path, int id, char *json_out, size_t max_size);
int dashboard_get_by_name(const char *db_path, const char *name, char *json_out, size_t max_size);
int dashboard_update(const char *db_path, int id, const char *title, const char *description);
int dashboard_delete(const char *db_path, int id);
int dashboard_set_default(const char *db_path, int id);

// 위젯 관리 함수
int widget_create(const char *db_path, int dashboard_id, const char *widget_type,
                  const char *title, const char *data_source, const char *query,
                  const char *config, int x, int y, int w, int h);
int widget_list(const char *db_path, int dashboard_id, char *json_out, size_t max_size);
int widget_update(const char *db_path, int widget_id, const char *title,
                  int x, int y, int w, int h);
int widget_delete(const char *db_path, int widget_id);
int widget_get_data(const char *db_path, int widget_id, char *json_out, size_t max_size);

// 쿼리 안전성 검증
int is_query_safe(const char *query);

#endif
