#include "aeon_monitor.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include <json-c/json.h>

#define GATEWAY_DB_PATH "../data/aeon_evolution.db"

// Gateway 상태 구조체
typedef struct {
    char server[32];           // 서버 주소
    int pid;                   // 프로세스 ID
    char phase[64];            // Phase A, B 등
    char status[32];           // Active Learning 등
    float uptime_days;         // 업타임
    int total_commands;        // 총 명령 수
    float success_rate;        // 성공률
    float block_rate;          // 차단률
    int memory_usage_mb;       // 메모리 사용량
    char level[8];             // L3, L4 등
    long timestamp;            // Unix 타임스탬프
} GatewayStats;

// SSH로 198 서버에서 상태 파일 가져오기
int fetch_gateway_status(GatewayStats *stats) {
    FILE *fp;
    char buffer[16384] = {0};

    // SSH 명령으로 상태 파일 읽기
    fp = popen("ssh -p 22 kim@192.168.45.198 'cat /home/kim/ai-gateway-198/AI_STATUS_SUMMARY.json' 2>/dev/null", "r");
    if (!fp) {
        fprintf(stderr, "[Gateway Monitor] SSH 실패\n");
        return -1;
    }

    // JSON 읽기
    size_t len = fread(buffer, 1, sizeof(buffer) - 1, fp);
    pclose(fp);

    if (len == 0) {
        fprintf(stderr, "[Gateway Monitor] 데이터 없음\n");
        return -1;
    }

    // JSON 파싱
    struct json_object *parsed_json = json_tokener_parse(buffer);
    if (!parsed_json) {
        fprintf(stderr, "[Gateway Monitor] JSON 파싱 실패\n");
        return -1;
    }

    // 데이터 추출
    struct json_object *server_obj, *learning_obj, *perf_obj, *sec_obj, *heartbeat_obj;

    json_object_object_get_ex(parsed_json, "server", &server_obj);
    json_object_object_get_ex(parsed_json, "learning_summary", &learning_obj);
    json_object_object_get_ex(parsed_json, "performance_metrics", &perf_obj);
    json_object_object_get_ex(parsed_json, "security_learning", &sec_obj);

    // 하트비트 파일에서 PID 가져오기
    fp = popen("ssh -p 22 kim@192.168.45.198 'cat /home/kim/ai-gateway-198/state/heartbeat.json' 2>/dev/null", "r");
    if (fp) {
        char hb_buffer[1024] = {0};
        fread(hb_buffer, 1, sizeof(hb_buffer) - 1, fp);
        pclose(fp);

        heartbeat_obj = json_tokener_parse(hb_buffer);
        if (heartbeat_obj) {
            struct json_object *pid_obj;
            json_object_object_get_ex(heartbeat_obj, "pid", &pid_obj);
            stats->pid = json_object_get_int(pid_obj);
            json_object_put(heartbeat_obj);
        }
    }

    // 값 복사
    if (server_obj) {
        strncpy(stats->server, json_object_get_string(server_obj), sizeof(stats->server) - 1);
    }

    if (learning_obj) {
        struct json_object *phase_obj, *status_obj, *uptime_obj, *cmds_obj, *success_obj, *block_obj;

        json_object_object_get_ex(learning_obj, "phase", &phase_obj);
        json_object_object_get_ex(learning_obj, "status", &status_obj);
        json_object_object_get_ex(learning_obj, "uptime_days", &uptime_obj);
        json_object_object_get_ex(learning_obj, "total_commands", &cmds_obj);
        json_object_object_get_ex(learning_obj, "success_rate", &success_obj);
        json_object_object_get_ex(learning_obj, "block_rate", &block_obj);

        if (phase_obj) strncpy(stats->phase, json_object_get_string(phase_obj), sizeof(stats->phase) - 1);
        if (status_obj) strncpy(stats->status, json_object_get_string(status_obj), sizeof(stats->status) - 1);
        if (uptime_obj) stats->uptime_days = json_object_get_double(uptime_obj);
        if (cmds_obj) stats->total_commands = json_object_get_int(cmds_obj);
        if (success_obj) stats->success_rate = json_object_get_double(success_obj);
        if (block_obj) stats->block_rate = json_object_get_double(block_obj);
    }

    if (perf_obj) {
        struct json_object *mem_obj;
        json_object_object_get_ex(perf_obj, "memory_usage_mb", &mem_obj);
        if (mem_obj) stats->memory_usage_mb = json_object_get_int(mem_obj);
    }

    if (sec_obj) {
        struct json_object *level_obj;
        json_object_object_get_ex(sec_obj, "level", &level_obj);
        if (level_obj) strncpy(stats->level, json_object_get_string(level_obj), sizeof(stats->level) - 1);
    }

    stats->timestamp = time(NULL);

    json_object_put(parsed_json);
    return 0;
}

// DB에 Gateway 테이블 생성
int init_gateway_table(const char *db_path) {
    sqlite3 *db;
    char *err_msg = NULL;
    int rc;

    rc = sqlite3_open(db_path, &db);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "[Gateway Monitor] DB 열기 실패: %s\n", sqlite3_errmsg(db));
        return -1;
    }

    const char *sql =
        "CREATE TABLE IF NOT EXISTS gateway_stats ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT, "
        "timestamp INTEGER NOT NULL, "
        "server TEXT NOT NULL, "
        "pid INTEGER, "
        "phase TEXT, "
        "status TEXT, "
        "uptime_days REAL, "
        "total_commands INTEGER, "
        "success_rate REAL, "
        "block_rate REAL, "
        "memory_usage_mb INTEGER, "
        "level TEXT);";

    rc = sqlite3_exec(db, sql, 0, 0, &err_msg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "[Gateway Monitor] 테이블 생성 실패: %s\n", err_msg);
        sqlite3_free(err_msg);
        sqlite3_close(db);
        return -1;
    }

    sqlite3_close(db);
    printf("[Gateway Monitor] 테이블 초기화 완료\n");
    return 0;
}

// DB에 Gateway 상태 기록
int log_gateway_stats(const char *db_path, GatewayStats *stats) {
    sqlite3 *db;
    char *err_msg = NULL;
    char sql[1024];
    int rc;

    rc = sqlite3_open(db_path, &db);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "[Gateway Monitor] DB 열기 실패\n");
        return -1;
    }

    snprintf(sql, sizeof(sql),
        "INSERT INTO gateway_stats (timestamp, server, pid, phase, status, "
        "uptime_days, total_commands, success_rate, block_rate, memory_usage_mb, level) "
        "VALUES (%ld, '%s', %d, '%s', '%s', %.2f, %d, %.2f, %.2f, %d, '%s');",
        stats->timestamp, stats->server, stats->pid, stats->phase, stats->status,
        stats->uptime_days, stats->total_commands, stats->success_rate, stats->block_rate,
        stats->memory_usage_mb, stats->level);

    rc = sqlite3_exec(db, sql, 0, 0, &err_msg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "[Gateway Monitor] 기록 실패: %s\n", err_msg);
        sqlite3_free(err_msg);
        sqlite3_close(db);
        return -1;
    }

    sqlite3_close(db);
    return 0;
}

// Gateway 상태 출력
void print_gateway_stats(GatewayStats *stats) {
    printf("[Gateway Stats] Server:%s | PID:%d | Phase:%s | Status:%s\n",
        stats->server, stats->pid, stats->phase, stats->status);
    printf("                Uptime:%.2f days | Commands:%d | Success:%.1f%% | Block:%.1f%%\n",
        stats->uptime_days, stats->total_commands, stats->success_rate, stats->block_rate);
    printf("                Memory:%dMB | Level:%s\n",
        stats->memory_usage_mb, stats->level);
}

#ifdef GATEWAY_MONITOR_EXAMPLE
int main() {
    const char *db_path = GATEWAY_DB_PATH;

    // 1. 테이블 초기화
    if (init_gateway_table(db_path) != 0) {
        return 1;
    }

    // 2. 무한 루프 (5초마다 상태 수집)
    printf("[Gateway Monitor] 시작 - 5초마다 198 서버 모니터링\n");
    printf("[Ctrl+C로 중지]\n\n");

    while (1) {
        GatewayStats stats = {0};

        if (fetch_gateway_status(&stats) == 0) {
            print_gateway_stats(&stats);
            log_gateway_stats(db_path, &stats);
        } else {
            fprintf(stderr, "[Gateway Monitor] 데이터 수집 실패\n");
        }

        sleep(5);
    }

    return 0;
}
#endif
