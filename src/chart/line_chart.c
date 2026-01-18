#include "chart.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

// 외부 선언 (svg_generator.c)
extern char* svg_line(double x1, double y1, double x2, double y2, const char *color, double width);
extern char* svg_circle(double cx, double cy, double r, const char *fill, const char *stroke);
extern char* svg_text(double x, double y, const char *text, const char *color, int size, const char *anchor);
extern char* svg_grid(int width, int height, int padding, int steps, const char *color);

// Line Chart 생성
char* chart_create_line(ChartData *data, ChartOptions *opts) {
    if (!data || !opts || data->count < 2) {
        fprintf(stderr, "Invalid data or options\n");
        return NULL;
    }

    // SVG 문자열 버퍼 (동적 확장)
    char *svg = svg_start(opts->width, opts->height, opts->bg_color);
    if (!svg) return NULL;

    int padding = opts->padding;
    int chart_width = opts->width - 2 * padding;
    int chart_height = opts->height - 2 * padding;

    // 데이터 최대/최소값 찾기
    double min_val = data->values[0];
    double max_val = data->values[0];
    for (int i = 1; i < data->count; i++) {
        if (data->values[i] < min_val) min_val = data->values[i];
        if (data->values[i] > max_val) max_val = data->values[i];
    }

    // 범위 여유 추가 (10%)
    double range = max_val - min_val;
    if (range < 0.0001) range = 1.0; // 모든 값이 같을 경우
    min_val -= range * 0.1;
    max_val += range * 0.1;
    range = max_val - min_val;

    // 그리드 그리기
    if (opts->show_grid) {
        char *grid = svg_grid(opts->width, opts->height, padding, 5, opts->grid_color);
        svg_append(&svg, grid);
        free(grid);
    }

    // 제목 그리기
    if (data->title && strlen(data->title) > 0) {
        char *title_text = svg_text(opts->width / 2.0, padding / 2.0,
                                     data->title, opts->text_color, 18, "middle");
        svg_append(&svg, title_text);
        free(title_text);
    }

    // Y축 눈금 그리기
    for (int i = 0; i <= 5; i++) {
        double value = min_val + (range * i / 5.0);
        double y = opts->height - padding - (chart_height * i / 5.0);

        char label[32];
        snprintf(label, sizeof(label), "%.2f", value);

        char *y_label = svg_text(padding - 10, y + 5, label, opts->text_color, 12, "end");
        svg_append(&svg, y_label);
        free(y_label);
    }

    // 선 그리기
    double step_x = (double)chart_width / (data->count - 1);

    for (int i = 0; i < data->count - 1; i++) {
        double x1 = padding + i * step_x;
        double x2 = padding + (i + 1) * step_x;

        double y1 = opts->height - padding -
                    ((data->values[i] - min_val) / range * chart_height);
        double y2 = opts->height - padding -
                    ((data->values[i + 1] - min_val) / range * chart_height);

        char *line = svg_line(x1, y1, x2, y2, data->color, 2.5);
        svg_append(&svg, line);
        free(line);
    }

    // 데이터 포인트 원 그리기
    for (int i = 0; i < data->count; i++) {
        double x = padding + i * step_x;
        double y = opts->height - padding -
                   ((data->values[i] - min_val) / range * chart_height);

        char *circle = svg_circle(x, y, 4, data->color, opts->bg_color);
        svg_append(&svg, circle);
        free(circle);
    }

    // X축 레이블 그리기
    if (data->labels) {
        for (int i = 0; i < data->count; i++) {
            double x = padding + i * step_x;
            double y = opts->height - padding + 20;

            char *x_label = svg_text(x, y, data->labels[i],
                                     opts->text_color, 11, "middle");
            svg_append(&svg, x_label);
            free(x_label);
        }
    }

    // SVG 종료
    char *end_tag = svg_end();
    svg_append(&svg, end_tag);
    free(end_tag);

    return svg;
}
