/**
 * Widget Renderer
 * 6가지 위젯 타입 렌더링 (stat, line-chart, bar-chart, table, gauge, log)
 */

class WidgetRenderer {
    constructor() {
        this.charts = {}; // Chart.js 인스턴스 저장
    }

    // 위젯 렌더링 (타입별 분기)
    async render(widget, container) {
        container.innerHTML = '<div class="widget-loading">Loading...</div>';

        try {
            const data = await API.widget.getData(widget.id);

            switch (widget.widget_type) {
                case 'stat':
                    this.renderStat(widget, container, data);
                    break;
                case 'line-chart':
                    this.renderLineChart(widget, container, data);
                    break;
                case 'bar-chart':
                    this.renderBarChart(widget, container, data);
                    break;
                case 'table':
                    this.renderTable(widget, container, data);
                    break;
                case 'gauge':
                    this.renderGauge(widget, container, data);
                    break;
                case 'log':
                    this.renderLog(widget, container, data);
                    break;
                default:
                    container.innerHTML = `<div class="widget-error">Unknown widget type: ${widget.widget_type}</div>`;
            }
        } catch (error) {
            container.innerHTML = `<div class="widget-error">Error: ${error.message}</div>`;
        }
    }

    // 1. Stat Widget (단일 값)
    renderStat(widget, container, data) {
        if (!data.success || !data.data || data.data.length === 0) {
            container.innerHTML = '<div class="widget-error">No data</div>';
            return;
        }

        const config = this.parseConfig(widget.config);
        const row = data.data[0];
        const keys = Object.keys(row);
        const value = row[keys[keys.length - 1]]; // 마지막 컬럼 값

        const formatted = this.formatValue(value, config.format, config.precision);
        const color = config.color || '#58a6ff';

        container.innerHTML = `
            <div class="stat-widget">
                <div class="stat-label">${widget.title}</div>
                <div class="stat-value" style="color: ${color}">
                    ${formatted}
                </div>
            </div>
        `;
    }

    // 2. Line Chart Widget (Server-side SVG)
    async renderLineChart(widget, container, data) {
        if (!data.success || !data.data || data.data.length === 0) {
            container.innerHTML = '<div class="widget-error">No data</div>';
            return;
        }

        container.innerHTML = '<div style="text-align: center; padding: 20px;">Loading chart...</div>';

        try {
            // Fetch SVG from server
            const response = await fetch(`/api/chart/line?widget_id=${widget.id}`);
            if (!response.ok) {
                throw new Error('Chart generation failed');
            }

            const svgText = await response.text();

            // Display SVG
            container.innerHTML = svgText;

            // Add responsive styling
            const svg = container.querySelector('svg');
            if (svg) {
                svg.style.width = '100%';
                svg.style.height = '100%';
            }
        } catch (error) {
            console.error('Line chart error:', error);
            container.innerHTML = '<div class="widget-error">Chart load failed</div>';
        }
    }

    // 3. Bar Chart Widget
    renderBarChart(widget, container, data) {
        if (!data.success || !data.data || data.data.length === 0) {
            container.innerHTML = '<div class="widget-error">No data</div>';
            return;
        }

        const config = this.parseConfig(widget.config);
        const canvas = document.createElement('canvas');
        container.innerHTML = '';
        container.appendChild(canvas);

        const keys = Object.keys(data.data[0]);
        const xField = config.x_field || keys[0];
        const yField = config.y_field || keys[1];

        const labels = data.data.map(row => row[xField]);
        const values = data.data.map(row => row[yField]);

        const chartId = `chart-${widget.id}`;
        if (this.charts[chartId]) {
            this.charts[chartId].destroy();
        }

        this.charts[chartId] = new Chart(canvas.getContext('2d'), {
            type: 'bar',
            data: {
                labels: labels,
                datasets: [{
                    label: widget.title,
                    data: values,
                    backgroundColor: config.color || '#58a6ff'
                }]
            },
            options: {
                responsive: true,
                maintainAspectRatio: false,
                plugins: {
                    legend: { display: false }
                },
                scales: {
                    x: {
                        grid: { color: '#30363d' },
                        ticks: { color: '#8b949e' }
                    },
                    y: {
                        grid: { color: '#30363d' },
                        ticks: { color: '#8b949e' }
                    }
                }
            }
        });
    }

    // 4. Table Widget
    renderTable(widget, container, data) {
        if (!data.success || !data.data || data.data.length === 0) {
            container.innerHTML = '<div class="widget-error">No data</div>';
            return;
        }

        const keys = Object.keys(data.data[0]);

        let html = '<div class="table-widget"><table><thead><tr>';
        keys.forEach(key => {
            html += `<th>${key}</th>`;
        });
        html += '</tr></thead><tbody>';

        data.data.forEach(row => {
            html += '<tr>';
            keys.forEach(key => {
                html += `<td>${row[key]}</td>`;
            });
            html += '</tr>';
        });

        html += '</tbody></table></div>';
        container.innerHTML = html;
    }

    // 5. Gauge Widget (0~100%)
    renderGauge(widget, container, data) {
        if (!data.success || !data.data || data.data.length === 0) {
            container.innerHTML = '<div class="widget-error">No data</div>';
            return;
        }

        const config = this.parseConfig(widget.config);
        const row = data.data[0];
        const keys = Object.keys(row);
        const value = parseFloat(row[keys[keys.length - 1]]);
        const percentage = Math.min(100, Math.max(0, value));

        const color = percentage > 80 ? '#3fb950' :
                      percentage > 50 ? '#d29922' : '#f85149';

        container.innerHTML = `
            <div class="gauge-widget">
                <div class="gauge-label">${widget.title}</div>
                <div class="gauge-bar">
                    <div class="gauge-fill" style="width: ${percentage}%; background: ${color}"></div>
                </div>
                <div class="gauge-value">${percentage.toFixed(1)}%</div>
            </div>
        `;
    }

    // 6. Log Widget
    renderLog(widget, container, data) {
        if (!data.success || !data.data || data.data.length === 0) {
            container.innerHTML = '<div class="widget-info">No logs</div>';
            return;
        }

        let html = '<div class="log-widget">';
        data.data.forEach(row => {
            const timestamp = row.timestamp
                ? new Date(row.timestamp * 1000).toLocaleString()
                : '';
            const message = row.error_msg || row.message || JSON.stringify(row);

            html += `
                <div class="log-item">
                    ${timestamp ? `<span class="log-time">${timestamp}</span>` : ''}
                    <span class="log-msg">${message}</span>
                </div>
            `;
        });
        html += '</div>';

        container.innerHTML = html;
    }

    // 헬퍼: Config 파싱
    parseConfig(configStr) {
        try {
            return typeof configStr === 'string' ? JSON.parse(configStr) : configStr;
        } catch {
            return {};
        }
    }

    // 헬퍼: 값 포맷팅
    formatValue(value, format, precision = 2) {
        if (value === null || value === undefined) return '-';

        switch (format) {
            case 'number':
                return parseInt(value).toLocaleString();
            case 'decimal':
                return parseFloat(value).toFixed(precision);
            case 'percentage':
                return parseFloat(value).toFixed(1) + '%';
            case 'text':
                return String(value);
            default:
                return value;
        }
    }

    // 모든 차트 정리
    destroyAll() {
        Object.values(this.charts).forEach(chart => chart.destroy());
        this.charts = {};
    }
}

// 전역 스코프에 노출
window.WidgetRenderer = WidgetRenderer;
