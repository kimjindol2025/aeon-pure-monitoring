#ifndef AEON_MONITOR_H
#define AEON_MONITOR_H

#include <sqlite3.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// 모니터링 상태 구조체
typedef struct {
    float fitness;          // 진화 적합도
    int generation;         // 세대 번호
    int workers;            // 작업자 수
    int errors;             // 에러 카운트
    char status[32];        // 상태 (running, paused, error)
    long timestamp;         // Unix 타임스탬프
} AeonStats;

// 함수 프로토타입
int aeon_monitor_init(const char *db_path);
int aeon_monitor_log(const char *db_path, AeonStats *stats);
int aeon_monitor_log_error(const char *db_path, const char *error_msg);
int aeon_monitor_cleanup(const char *db_path);
void aeon_monitor_print_stats(AeonStats *stats);

#endif // AEON_MONITOR_H
