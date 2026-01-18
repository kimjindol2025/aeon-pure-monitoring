#include "chart.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// 기본 옵션 생성
ChartOptions chart_default_options(void) {
    ChartOptions opts = {
        .width = 800,
        .height = 400,
        .bg_color = "#161b22",
        .text_color = "#c9d1d9",
        .grid_color = "#30363d",
        .show_grid = 1,
        .show_legend = 0,
        .padding = 60
    };
    return opts;
}

// 차트 생성 (타입별 분기)
char* chart_create(ChartType type, ChartData *data, ChartOptions *opts) {
    if (!data || !opts) {
        fprintf(stderr, "chart_create: NULL data or options\n");
        return NULL;
    }

    switch (type) {
        case CHART_LINE:
            return chart_create_line(data, opts);

        case CHART_BAR:
            fprintf(stderr, "Bar chart not implemented yet\n");
            return NULL;

        case CHART_PIE:
            fprintf(stderr, "Pie chart not implemented yet\n");
            return NULL;

        case CHART_DOUGHNUT:
            fprintf(stderr, "Doughnut chart not implemented yet\n");
            return NULL;

        case CHART_AREA:
            fprintf(stderr, "Area chart not implemented yet\n");
            return NULL;

        default:
            fprintf(stderr, "Unknown chart type: %d\n", type);
            return NULL;
    }
}

// SVG 파일로 저장
int chart_save(const char *svg_content, const char *filepath) {
    if (!svg_content || !filepath) {
        fprintf(stderr, "chart_save: NULL content or filepath\n");
        return -1;
    }

    FILE *file = fopen(filepath, "w");
    if (!file) {
        perror("chart_save: fopen failed");
        return -1;
    }

    size_t written = fwrite(svg_content, 1, strlen(svg_content), file);
    fclose(file);

    if (written != strlen(svg_content)) {
        fprintf(stderr, "chart_save: incomplete write\n");
        return -1;
    }

    return 0;
}

// 메모리 해제
void chart_free(char *svg_content) {
    if (svg_content) {
        free(svg_content);
    }
}
