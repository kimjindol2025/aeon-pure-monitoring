/**
 * Grid Layout Manager
 * CSS Grid 기반 12 컬럼 레이아웃
 */

class GridLayout {
    constructor(containerId, columns = 12) {
        this.container = document.getElementById(containerId);
        this.columns = columns;
        this.widgets = [];
    }

    // 위젯 추가
    addWidget(widget) {
        const widgetEl = document.createElement('div');
        widgetEl.className = 'widget-container';
        widgetEl.id = `widget-${widget.id}`;
        widgetEl.style.gridColumn = `span ${widget.width}`;
        widgetEl.style.gridRow = `span ${widget.height}`;

        // 위젯 헤더
        const header = document.createElement('div');
        header.className = 'widget-header';
        header.innerHTML = `
            <span class="widget-title">${widget.title}</span>
            <button class="widget-delete" onclick="deleteWidget(${widget.id})">×</button>
        `;

        // 위젯 본문
        const body = document.createElement('div');
        body.className = 'widget-body';
        body.id = `widget-body-${widget.id}`;

        widgetEl.appendChild(header);
        widgetEl.appendChild(body);
        this.container.appendChild(widgetEl);

        this.widgets.push({ widget, element: widgetEl });
        return body;
    }

    // 모든 위젯 제거
    clear() {
        this.container.innerHTML = '';
        this.widgets = [];
    }

    // 위젯 찾기
    getWidget(widgetId) {
        return this.widgets.find(w => w.widget.id === widgetId);
    }

    // 위젯 제거
    removeWidget(widgetId) {
        const widget = this.getWidget(widgetId);
        if (widget) {
            widget.element.remove();
            this.widgets = this.widgets.filter(w => w.widget.id !== widgetId);
        }
    }
}

// 전역 스코프에 노출
window.GridLayout = GridLayout;
