#ifndef CHART_H
#define CHART_H

#include <stddef.h>

// 차트 타입
typedef enum {
    CHART_LINE,      // 선 그래프
    CHART_BAR,       // 막대 그래프
    CHART_PIE,       // 파이 차트
    CHART_DOUGHNUT,  // 도넛 차트
    CHART_AREA       // 영역 차트
} ChartType;

// 차트 데이터
typedef struct {
    double *values;     // 데이터 값 배열
    char **labels;      // 레이블 배열
    int count;          // 데이터 개수
    char *title;        // 차트 제목
    char *color;        // 주 색상 (hex, 예: "#58a6ff")
} ChartData;

// 차트 옵션
typedef struct {
    int width;          // 너비 (px)
    int height;         // 높이 (px)
    char *bg_color;     // 배경색 (hex)
    char *text_color;   // 텍스트 색 (hex)
    char *grid_color;   // 그리드 색 (hex)
    int show_grid;      // 그리드 표시 (1: 표시, 0: 숨김)
    int show_legend;    // 범례 표시
    int padding;        // 패딩 (px)
} ChartOptions;

// 차트 생성 (SVG 문자열 반환)
char* chart_create(ChartType type, ChartData *data, ChartOptions *opts);

// SVG 파일로 저장
int chart_save(const char *svg_content, const char *filepath);

// 메모리 해제
void chart_free(char *svg_content);

// 기본 옵션 생성
ChartOptions chart_default_options(void);

// Line Chart 전용 함수 (내부용)
char* chart_create_line(ChartData *data, ChartOptions *opts);

// SVG 생성 유틸리티 (내부용)
char* svg_start(int width, int height, const char *bg_color);
void svg_append(char **dest, const char *src);
char* svg_end(void);

#endif // CHART_H
