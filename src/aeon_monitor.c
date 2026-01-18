#include "aeon_monitor.h"

/**
 * AEON Pure Monitoring System
 * 철학: 0% 외부 의존성, 기술 주권
 * 구조: C → SQLite → Grafana (Zero Middleware)
 */

// DB 초기화 (테이블 생성)
int aeon_monitor_init(const char *db_path) {
    sqlite3 *db;
    char *err_msg = NULL;
    int rc;

    rc = sqlite3_open(db_path, &db);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "[AEON Monitor] DB 열기 실패: %s\n", sqlite3_errmsg(db));
        return -1;
    }

    // 진화 통계 테이블
    const char *sql_stats =
        "CREATE TABLE IF NOT EXISTS evolution_stats ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT, "
        "timestamp INTEGER NOT NULL, "
        "fitness REAL NOT NULL, "
        "generation INTEGER NOT NULL, "
        "workers INTEGER NOT NULL, "
        "errors INTEGER DEFAULT 0, "
        "status TEXT DEFAULT 'running');";

    rc = sqlite3_exec(db, sql_stats, 0, 0, &err_msg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "[AEON Monitor] 테이블 생성 실패: %s\n", err_msg);
        sqlite3_free(err_msg);
        sqlite3_close(db);
        return -1;
    }

    // 에러 로그 테이블
    const char *sql_errors =
        "CREATE TABLE IF NOT EXISTS error_logs ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT, "
        "timestamp INTEGER NOT NULL, "
        "error_msg TEXT NOT NULL);";

    rc = sqlite3_exec(db, sql_errors, 0, 0, &err_msg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "[AEON Monitor] 에러 테이블 생성 실패: %s\n", err_msg);
        sqlite3_free(err_msg);
        sqlite3_close(db);
        return -1;
    }

    sqlite3_close(db);
    printf("[AEON Monitor] DB 초기화 완료: %s\n", db_path);
    return 0;
}

// 진화 통계 기록
int aeon_monitor_log(const char *db_path, AeonStats *stats) {
    sqlite3 *db;
    char *err_msg = NULL;
    char sql[512];
    int rc;

    // 현재 시간 설정 (미지정 시)
    if (stats->timestamp == 0) {
        stats->timestamp = time(NULL);
    }

    rc = sqlite3_open(db_path, &db);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "[AEON Monitor] DB 열기 실패: %s\n", sqlite3_errmsg(db));
        return -1;
    }

    // INSERT 쿼리 생성
    snprintf(sql, sizeof(sql),
        "INSERT INTO evolution_stats (timestamp, fitness, generation, workers, errors, status) "
        "VALUES (%ld, %.6f, %d, %d, %d, '%s');",
        stats->timestamp, stats->fitness, stats->generation,
        stats->workers, stats->errors, stats->status);

    rc = sqlite3_exec(db, sql, 0, 0, &err_msg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "[AEON Monitor] 통계 기록 실패: %s\n", err_msg);
        sqlite3_free(err_msg);
        sqlite3_close(db);
        return -1;
    }

    sqlite3_close(db);
    return 0;
}

// 에러 로그 기록
int aeon_monitor_log_error(const char *db_path, const char *error_msg) {
    sqlite3 *db;
    char *err = NULL;
    char sql[1024];
    int rc;
    long timestamp = time(NULL);

    rc = sqlite3_open(db_path, &db);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "[AEON Monitor] DB 열기 실패: %s\n", sqlite3_errmsg(db));
        return -1;
    }

    snprintf(sql, sizeof(sql),
        "INSERT INTO error_logs (timestamp, error_msg) VALUES (%ld, '%s');",
        timestamp, error_msg);

    rc = sqlite3_exec(db, sql, 0, 0, &err);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "[AEON Monitor] 에러 기록 실패: %s\n", err);
        sqlite3_free(err);
        sqlite3_close(db);
        return -1;
    }

    sqlite3_close(db);
    printf("[AEON Monitor] 에러 기록: %s\n", error_msg);
    return 0;
}

// DB 정리 (오래된 데이터 삭제)
int aeon_monitor_cleanup(const char *db_path) {
    sqlite3 *db;
    char *err_msg = NULL;
    int rc;
    long cutoff = time(NULL) - (7 * 24 * 60 * 60); // 7일 전

    rc = sqlite3_open(db_path, &db);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "[AEON Monitor] DB 열기 실패: %s\n", sqlite3_errmsg(db));
        return -1;
    }

    char sql[256];
    snprintf(sql, sizeof(sql),
        "DELETE FROM evolution_stats WHERE timestamp < %ld;", cutoff);

    rc = sqlite3_exec(db, sql, 0, 0, &err_msg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "[AEON Monitor] 정리 실패: %s\n", err_msg);
        sqlite3_free(err_msg);
        sqlite3_close(db);
        return -1;
    }

    sqlite3_close(db);
    printf("[AEON Monitor] 7일 이전 데이터 정리 완료\n");
    return 0;
}

// 통계 출력 (디버깅용)
void aeon_monitor_print_stats(AeonStats *stats) {
    printf("[AEON Stats] Gen:%d | Fitness:%.4f | Workers:%d | Errors:%d | Status:%s\n",
        stats->generation, stats->fitness, stats->workers,
        stats->errors, stats->status);
}

// 사용 예시
#ifdef AEON_MONITOR_EXAMPLE
int main() {
    const char *db_path = "../data/aeon_evolution.db";

    // 1. DB 초기화
    if (aeon_monitor_init(db_path) != 0) {
        return 1;
    }

    // 2. 진화 통계 기록 (시뮬레이션)
    for (int i = 0; i < 10; i++) {
        AeonStats stats = {
            .fitness = 0.5 + (i * 0.05),
            .generation = i,
            .workers = 4,
            .errors = 0,
            .status = "running",
            .timestamp = 0
        };

        aeon_monitor_print_stats(&stats);
        aeon_monitor_log(db_path, &stats);
        sleep(1);
    }

    // 3. 에러 시뮬레이션
    aeon_monitor_log_error(db_path, "Test error: Worker 2 crashed");

    // 4. 정리
    aeon_monitor_cleanup(db_path);

    printf("[AEON Monitor] 테스트 완료\n");
    return 0;
}
#endif
