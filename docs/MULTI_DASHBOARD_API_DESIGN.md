# Multi Dashboard API 설계

## 개요

단일 대시보드에서 멀티 대시보드 시스템으로 확장. 사용자가 여러 개의 대시보드를 생성하고 각각에 위젯을 추가할 수 있는 구조.

## 데이터베이스 스키마

### 1. dashboards 테이블

```sql
CREATE TABLE dashboards (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    name TEXT NOT NULL UNIQUE,
    title TEXT NOT NULL,
    description TEXT,
    layout TEXT DEFAULT 'grid',  -- 'grid', 'flex', 'custom'
    created_at INTEGER NOT NULL,
    updated_at INTEGER NOT NULL,
    is_default INTEGER DEFAULT 0  -- 기본 대시보드 플래그
);
```

### 2. widgets 테이블

```sql
CREATE TABLE widgets (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    dashboard_id INTEGER NOT NULL,
    widget_type TEXT NOT NULL,  -- 'stat', 'chart', 'table', 'log'
    title TEXT NOT NULL,
    data_source TEXT NOT NULL,  -- 'evolution_stats', 'gateway_stats', 'error_logs'
    query TEXT,  -- SQL 쿼리 또는 데이터 소스 설정
    config TEXT,  -- JSON 형태의 위젯 설정
    position_x INTEGER DEFAULT 0,
    position_y INTEGER DEFAULT 0,
    width INTEGER DEFAULT 4,
    height INTEGER DEFAULT 3,
    created_at INTEGER NOT NULL,
    FOREIGN KEY (dashboard_id) REFERENCES dashboards(id) ON DELETE CASCADE
);
```

### 3. widget_types 정의

| Type | 설명 | data_source 예시 |
|------|------|------------------|
| `stat` | 단일 값 표시 | `latest_fitness`, `gateway_uptime` |
| `line-chart` | 시계열 차트 | `fitness_history`, `command_count` |
| `bar-chart` | 막대 차트 | `worker_distribution` |
| `table` | 테이블 | `error_logs`, `gateway_commands` |
| `gauge` | 게이지 | `success_rate`, `cpu_usage` |
| `log` | 로그 뷰어 | `error_logs`, `system_logs` |

## RESTful API 설계

### Dashboard APIs

#### 1. 대시보드 목록 조회
```http
GET /api/dashboards
```

**응답:**
```json
{
  "success": true,
  "dashboards": [
    {
      "id": 1,
      "name": "overview",
      "title": "Overview",
      "description": "AEON + Gateway 통합 뷰",
      "layout": "grid",
      "widget_count": 12,
      "is_default": 1,
      "created_at": 1768742000
    },
    {
      "id": 2,
      "name": "gateway-detail",
      "title": "AI Gateway 198 상세",
      "layout": "grid",
      "widget_count": 8,
      "is_default": 0,
      "created_at": 1768742100
    }
  ]
}
```

#### 2. 대시보드 생성
```http
POST /api/dashboards
Content-Type: application/json

{
  "name": "custom-dashboard",
  "title": "My Custom Dashboard",
  "description": "설명",
  "layout": "grid"
}
```

**응답:**
```json
{
  "success": true,
  "dashboard_id": 3,
  "name": "custom-dashboard"
}
```

#### 3. 대시보드 조회 (위젯 포함)
```http
GET /api/dashboards/{id}
또는
GET /api/dashboards/by-name/{name}
```

**응답:**
```json
{
  "success": true,
  "dashboard": {
    "id": 1,
    "name": "overview",
    "title": "Overview",
    "description": "AEON + Gateway 통합",
    "layout": "grid",
    "created_at": 1768742000,
    "updated_at": 1768742500
  },
  "widgets": [
    {
      "id": 1,
      "widget_type": "stat",
      "title": "Current Generation",
      "data_source": "evolution_stats",
      "query": "SELECT generation FROM evolution_stats ORDER BY timestamp DESC LIMIT 1",
      "config": {
        "format": "number",
        "color": "#58a6ff"
      },
      "position_x": 0,
      "position_y": 0,
      "width": 3,
      "height": 2
    },
    {
      "id": 2,
      "widget_type": "line-chart",
      "title": "Fitness Evolution",
      "data_source": "evolution_stats",
      "query": "SELECT timestamp, fitness FROM evolution_stats ORDER BY timestamp DESC LIMIT 50",
      "config": {
        "x_field": "timestamp",
        "y_field": "fitness",
        "color": "#58a6ff"
      },
      "position_x": 0,
      "position_y": 2,
      "width": 12,
      "height": 4
    }
  ]
}
```

#### 4. 대시보드 수정
```http
PUT /api/dashboards/{id}
Content-Type: application/json

{
  "title": "수정된 제목",
  "description": "수정된 설명"
}
```

#### 5. 대시보드 삭제
```http
DELETE /api/dashboards/{id}
```

#### 6. 기본 대시보드 설정
```http
POST /api/dashboards/{id}/set-default
```

### Widget APIs

#### 1. 위젯 추가
```http
POST /api/dashboards/{id}/widgets
Content-Type: application/json

{
  "widget_type": "stat",
  "title": "Success Rate",
  "data_source": "gateway_stats",
  "query": "SELECT success_rate FROM gateway_stats ORDER BY timestamp DESC LIMIT 1",
  "config": {
    "format": "percentage",
    "color": "#3fb950"
  },
  "position_x": 3,
  "position_y": 0,
  "width": 3,
  "height": 2
}
```

**응답:**
```json
{
  "success": true,
  "widget_id": 15
}
```

#### 2. 위젯 수정
```http
PUT /api/widgets/{id}
Content-Type: application/json

{
  "title": "새 제목",
  "position_x": 6,
  "position_y": 0,
  "width": 4
}
```

#### 3. 위젯 삭제
```http
DELETE /api/widgets/{id}
```

#### 4. 위젯 순서 변경 (Bulk Update)
```http
POST /api/dashboards/{id}/widgets/reorder
Content-Type: application/json

{
  "widgets": [
    {"id": 1, "position_x": 0, "position_y": 0},
    {"id": 2, "position_x": 3, "position_y": 0},
    {"id": 3, "position_x": 6, "position_y": 0}
  ]
}
```

### Data APIs (위젯용)

#### 1. 위젯 데이터 조회
```http
GET /api/widgets/{id}/data
또는
POST /api/query
Content-Type: application/json

{
  "data_source": "evolution_stats",
  "query": "SELECT * FROM evolution_stats ORDER BY timestamp DESC LIMIT 10"
}
```

**응답:**
```json
{
  "success": true,
  "data": [
    {"timestamp": 1768742500, "fitness": 0.95, "generation": 9},
    {"timestamp": 1768742400, "fitness": 0.90, "generation": 8}
  ]
}
```

#### 2. 미리 정의된 데이터 소스
```http
GET /api/data-sources
```

**응답:**
```json
{
  "success": true,
  "data_sources": [
    {
      "name": "latest_evolution",
      "description": "최신 AEON 상태",
      "table": "evolution_stats",
      "query_template": "SELECT * FROM evolution_stats ORDER BY timestamp DESC LIMIT 1"
    },
    {
      "name": "fitness_history",
      "description": "적합도 히스토리",
      "table": "evolution_stats",
      "query_template": "SELECT timestamp, fitness FROM evolution_stats ORDER BY timestamp DESC LIMIT 50"
    },
    {
      "name": "latest_gateway",
      "description": "최신 Gateway 상태",
      "table": "gateway_stats",
      "query_template": "SELECT * FROM gateway_stats ORDER BY timestamp DESC LIMIT 1"
    }
  ]
}
```

## 프론트엔드 구조

### 1. 메인 페이지 (`/`)

```
+--------------------------------------------------+
| AEON Pure Monitoring                    [+ New]  |
+--------------------------------------------------+
| Dashboards: [Overview] [Gateway] [Custom] ...    |
+--------------------------------------------------+
|                                                  |
|  [Grid Layout - 12 columns]                      |
|                                                  |
|  +----------+  +----------+  +----------+        |
|  | Widget 1 |  | Widget 2 |  | Widget 3 |        |
|  |  Stat    |  |  Stat    |  |  Stat    |        |
|  +----------+  +----------+  +----------+        |
|                                                  |
|  +------------------------------------+          |
|  | Widget 4 - Line Chart              |          |
|  |                                    |          |
|  +------------------------------------+          |
|                                                  |
+--------------------------------------------------+
```

### 2. 대시보드 생성 UI

```
+--------------------------------------------------+
| Create New Dashboard                        [X]  |
+--------------------------------------------------+
| Name:        [_____________________________]     |
| Title:       [_____________________________]     |
| Description: [_____________________________]     |
| Layout:      ( ) Grid  ( ) Flex                  |
|                                                  |
|              [Cancel]  [Create Dashboard]        |
+--------------------------------------------------+
```

### 3. 위젯 추가 UI

```
+--------------------------------------------------+
| Add Widget to Dashboard                     [X]  |
+--------------------------------------------------+
| Type:        [Stat ▼]                            |
|              - Stat                              |
|              - Line Chart                        |
|              - Bar Chart                         |
|              - Table                             |
|              - Gauge                             |
|              - Log                               |
|                                                  |
| Title:       [_____________________________]     |
|                                                  |
| Data Source: [Latest Evolution ▼]                |
|              - Latest Evolution                  |
|              - Fitness History                   |
|              - Latest Gateway                    |
|              - Custom Query                      |
|                                                  |
| Position:    X [0] Y [0] Width [4] Height [3]    |
|                                                  |
|              [Cancel]  [Add Widget]              |
+--------------------------------------------------+
```

## 위젯 렌더링 시스템

### JavaScript 구조

```javascript
// widget-renderer.js
class WidgetRenderer {
  static render(widget, container) {
    switch(widget.widget_type) {
      case 'stat':
        return this.renderStat(widget, container);
      case 'line-chart':
        return this.renderLineChart(widget, container);
      case 'bar-chart':
        return this.renderBarChart(widget, container);
      case 'table':
        return this.renderTable(widget, container);
      case 'gauge':
        return this.renderGauge(widget, container);
      case 'log':
        return this.renderLog(widget, container);
    }
  }

  static async fetchData(widget) {
    const response = await fetch(`/api/widgets/${widget.id}/data`);
    return await response.json();
  }

  static renderStat(widget, container) {
    const data = await this.fetchData(widget);
    const value = data.data[0][widget.config.field];
    const html = `
      <div class="stat-widget">
        <div class="label">${widget.title}</div>
        <div class="value" style="color: ${widget.config.color}">
          ${this.formatValue(value, widget.config.format)}
        </div>
      </div>
    `;
    container.innerHTML = html;
  }
}
```

## 파일 구조

```
aeon-pure-monitoring/
├── src/
│   ├── api_server.c          # 확장: 새 API 추가
│   ├── dashboard_manager.c   # 신규: 대시보드 관리
│   ├── widget_manager.c      # 신규: 위젯 관리
│   └── Makefile
├── web/
│   ├── index.html            # 신규: 메인 (대시보드 선택)
│   ├── dashboard.html        # 신규: 대시보드 뷰어
│   ├── js/
│   │   ├── dashboard-manager.js  # 대시보드 관리
│   │   ├── widget-renderer.js    # 위젯 렌더링
│   │   └── grid-layout.js        # 그리드 레이아웃
│   └── css/
│       └── dashboard.css
└── data/
    └── aeon_evolution.db     # 스키마 확장
```

## 초기 데이터 (Migration)

```sql
-- 기본 대시보드 생성
INSERT INTO dashboards (name, title, description, layout, created_at, updated_at, is_default)
VALUES ('overview', 'Overview', 'AEON Evolution + AI Gateway 통합', 'grid',
        strftime('%s', 'now'), strftime('%s', 'now'), 1);

-- AEON 위젯들
INSERT INTO widgets (dashboard_id, widget_type, title, data_source, query, config,
                     position_x, position_y, width, height, created_at)
VALUES
  (1, 'stat', 'Current Generation', 'evolution_stats',
   'SELECT generation FROM evolution_stats ORDER BY timestamp DESC LIMIT 1',
   '{"format":"number","color":"#58a6ff"}', 0, 0, 3, 2, strftime('%s', 'now')),

  (1, 'stat', 'Fitness Score', 'evolution_stats',
   'SELECT fitness FROM evolution_stats ORDER BY timestamp DESC LIMIT 1',
   '{"format":"decimal","precision":4,"color":"#58a6ff"}', 3, 0, 3, 2, strftime('%s', 'now')),

  (1, 'line-chart', 'Fitness Evolution', 'evolution_stats',
   'SELECT timestamp, fitness FROM evolution_stats ORDER BY timestamp DESC LIMIT 50',
   '{"x_field":"timestamp","y_field":"fitness","color":"#58a6ff"}',
   0, 2, 12, 4, strftime('%s', 'now'));

-- Gateway 위젯들
INSERT INTO widgets (dashboard_id, widget_type, title, data_source, query, config,
                     position_x, position_y, width, height, created_at)
VALUES
  (1, 'stat', 'Gateway Server', 'gateway_stats',
   'SELECT server FROM gateway_stats ORDER BY timestamp DESC LIMIT 1',
   '{"format":"text","color":"#58a6ff"}', 0, 6, 3, 2, strftime('%s', 'now')),

  (1, 'stat', 'Success Rate', 'gateway_stats',
   'SELECT success_rate FROM gateway_stats ORDER BY timestamp DESC LIMIT 1',
   '{"format":"percentage","color":"#3fb950"}', 3, 6, 3, 2, strftime('%s', 'now'));
```

## 구현 단계

### Phase 1: 백엔드 (C)
- [ ] dashboards, widgets 테이블 생성
- [ ] dashboard CRUD API 구현
- [ ] widget CRUD API 구현
- [ ] 동적 쿼리 실행 엔진

### Phase 2: 프론트엔드 (HTML/JS)
- [ ] 대시보드 목록 UI
- [ ] 대시보드 생성/수정/삭제 UI
- [ ] 그리드 레이아웃 시스템
- [ ] 위젯 렌더러 (6가지 타입)
- [ ] 위젯 추가/수정/삭제 UI
- [ ] 드래그 앤 드롭 (선택)

### Phase 3: 마이그레이션
- [ ] 현재 dashboard.html → 위젯으로 변환
- [ ] 기본 대시보드 데이터 삽입
- [ ] 하위 호환성 유지 (`/` → 기본 대시보드)

## 예상 코드 규모

| 파일 | 예상 줄 수 | 설명 |
|------|----------|------|
| dashboard_manager.c | ~300줄 | 대시보드 CRUD |
| widget_manager.c | ~400줄 | 위젯 CRUD + 쿼리 실행 |
| api_server.c (확장) | +200줄 | 새 API 라우팅 |
| index.html | ~150줄 | 대시보드 선택 |
| dashboard.html (신규) | ~400줄 | 동적 대시보드 뷰어 |
| dashboard-manager.js | ~250줄 | 대시보드 관리 로직 |
| widget-renderer.js | ~350줄 | 위젯 렌더링 |
| grid-layout.js | ~200줄 | 그리드 시스템 |
| **총** | **~2,250줄** | |

## 기술적 도전

1. **SQL Injection 방지**: Prepared Statement 사용
2. **동적 쿼리 실행**: SQLite 직접 실행 (제한된 권한)
3. **그리드 레이아웃**: CSS Grid 사용
4. **실시간 업데이트**: 기존 5초 폴링 유지
5. **상태 관리**: JavaScript 객체로 간단하게

## 보안 고려사항

- ❌ 임의 SQL 실행 허용 (위험)
- ✅ 화이트리스트 기반 테이블 접근
- ✅ SELECT만 허용 (INSERT/UPDATE/DELETE 금지)
- ✅ 입력값 검증 (SQL Injection 방지)

```c
// 허용된 테이블만 쿼리 가능
const char *allowed_tables[] = {
    "evolution_stats",
    "gateway_stats",
    "error_logs",
    NULL
};

int is_query_safe(const char *query) {
    // SELECT만 허용
    if (strncasecmp(query, "SELECT", 6) != 0) return 0;

    // 허용 테이블 검증
    for (int i = 0; allowed_tables[i]; i++) {
        if (strstr(query, allowed_tables[i])) return 1;
    }
    return 0;
}
```

## 구현 진행 여부를 확인해주세요.
