/**
 * AEON Pure Monitoring API Helper
 * 순수 JavaScript API 클라이언트 (외부 의존성 0%)
 */

const API = {
    baseUrl: '',

    // HTTP 요청 헬퍼
    async request(method, endpoint, data = null) {
        const options = {
            method: method,
            headers: {
                'Content-Type': 'application/json'
            }
        };

        if (data && (method === 'POST' || method === 'PUT')) {
            options.body = JSON.stringify(data);
        }

        try {
            const response = await fetch(this.baseUrl + endpoint, options);
            const result = await response.json();

            if (!response.ok) {
                throw new Error(result.error || `HTTP ${response.status}`);
            }

            return result;
        } catch (error) {
            console.error(`API Error [${method} ${endpoint}]:`, error);
            throw error;
        }
    },

    // Dashboard APIs
    dashboard: {
        // 대시보드 목록 조회
        async list() {
            return await API.request('GET', '/api/dashboards');
        },

        // 대시보드 생성
        async create(name, title, description = '', layout = 'grid') {
            return await API.request('POST', '/api/dashboards', {
                name, title, description, layout
            });
        },

        // 대시보드 조회 (위젯 포함)
        async get(id) {
            return await API.request('GET', `/api/dashboards/${id}`);
        },

        // 대시보드 삭제
        async delete(id) {
            return await API.request('DELETE', `/api/dashboards/${id}`);
        },

        // 기본 대시보드 설정
        async setDefault(id) {
            return await API.request('POST', `/api/dashboards/${id}/set-default`);
        }
    },

    // Widget APIs
    widget: {
        // 위젯 추가
        async create(dashboardId, widgetData) {
            const {
                widget_type,
                title,
                data_source,
                query = '',
                config = {},
                position_x = 0,
                position_y = 0,
                width = 4,
                height = 3
            } = widgetData;

            return await API.request('POST', `/api/dashboards/${dashboardId}/widgets`, {
                widget_type,
                title,
                data_source,
                query,
                config: JSON.stringify(config),
                position_x,
                position_y,
                width,
                height
            });
        },

        // 위젯 삭제
        async delete(widgetId) {
            return await API.request('DELETE', `/api/widgets/${widgetId}`);
        },

        // 위젯 데이터 조회
        async getData(widgetId) {
            return await API.request('GET', `/api/widgets/${widgetId}/data`);
        }
    },

    // Legacy Stats API (하위 호환)
    stats: {
        async get() {
            return await API.request('GET', '/api/stats');
        }
    }
};

// 전역 스코프에 API 노출
window.API = API;
