#define _POSIX_C_SOURCE 200809L
#include "chart.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// SVG 시작 태그 생성
char* svg_start(int width, int height, const char *bg_color) {
    char *buffer = malloc(1024);
    if (!buffer) return NULL;

    snprintf(buffer, 1024,
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
        "<svg width=\"%d\" height=\"%d\" xmlns=\"http://www.w3.org/2000/svg\">\n"
        "  <rect width=\"%d\" height=\"%d\" fill=\"%s\"/>\n",
        width, height, width, height, bg_color);

    return buffer;
}

// SVG에 문자열 추가 (동적 확장)
void svg_append(char **dest, const char *src) {
    if (!dest || !src) return;

    if (*dest == NULL) {
        *dest = strdup(src);
        return;
    }

    size_t old_len = strlen(*dest);
    size_t new_len = old_len + strlen(src) + 1;

    char *temp = realloc(*dest, new_len);
    if (!temp) {
        fprintf(stderr, "svg_append: realloc failed\n");
        return;
    }

    *dest = temp;
    strcat(*dest, src);
}

// SVG 종료 태그
char* svg_end(void) {
    return strdup("</svg>\n");
}

// SVG 선 그리기
char* svg_line(double x1, double y1, double x2, double y2, const char *color, double width) {
    char *buffer = malloc(512);
    if (!buffer) return NULL;

    snprintf(buffer, 512,
        "  <line x1=\"%.2f\" y1=\"%.2f\" x2=\"%.2f\" y2=\"%.2f\" "
        "stroke=\"%s\" stroke-width=\"%.2f\"/>\n",
        x1, y1, x2, y2, color, width);

    return buffer;
}

// SVG 원 그리기
char* svg_circle(double cx, double cy, double r, const char *fill, const char *stroke) {
    char *buffer = malloc(512);
    if (!buffer) return NULL;

    snprintf(buffer, 512,
        "  <circle cx=\"%.2f\" cy=\"%.2f\" r=\"%.2f\" fill=\"%s\" stroke=\"%s\" stroke-width=\"2\"/>\n",
        cx, cy, r, fill, stroke);

    return buffer;
}

// SVG 텍스트 그리기
char* svg_text(double x, double y, const char *text, const char *color, int size, const char *anchor) {
    char *buffer = malloc(512);
    if (!buffer) return NULL;

    snprintf(buffer, 512,
        "  <text x=\"%.2f\" y=\"%.2f\" fill=\"%s\" font-size=\"%d\" "
        "font-family=\"Arial, sans-serif\" text-anchor=\"%s\">%s</text>\n",
        x, y, color, size, anchor, text);

    return buffer;
}

// SVG 경로 시작
char* svg_path_start(const char *color, double width, const char *fill) {
    char *buffer = malloc(512);
    if (!buffer) return NULL;

    snprintf(buffer, 512,
        "  <path d=\"M ");

    // color, width, fill은 추후 path 구현 시 사용
    (void)color;
    (void)width;
    (void)fill;

    return buffer;
}

// SVG 그리드 그리기
char* svg_grid(int width, int height, int padding, int steps, const char *color) {
    char *grid = malloc(8192);
    if (!grid) return NULL;

    grid[0] = '\0';
    char line[256];

    int chart_width = width - 2 * padding;
    int chart_height = height - 2 * padding;

    // 수평선
    for (int i = 0; i <= steps; i++) {
        double y = padding + (chart_height * i / steps);
        snprintf(line, sizeof(line),
            "  <line x1=\"%d\" y1=\"%.2f\" x2=\"%d\" y2=\"%.2f\" "
            "stroke=\"%s\" stroke-width=\"1\" opacity=\"0.3\"/>\n",
            padding, y, width - padding, y, color);
        strcat(grid, line);
    }

    // 수직선
    for (int i = 0; i <= steps; i++) {
        double x = padding + (chart_width * i / steps);
        snprintf(line, sizeof(line),
            "  <line x1=\"%.2f\" y1=\"%d\" x2=\"%.2f\" y2=\"%d\" "
            "stroke=\"%s\" stroke-width=\"1\" opacity=\"0.3\"/>\n",
            x, padding, x, height - padding, color);
        strcat(grid, line);
    }

    return grid;
}
