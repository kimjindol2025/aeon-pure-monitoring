# AEON Pure Monitoring

**철학: 0% 외부 의존성 | 기술 주권 | 순수 C 구현**

AEON 진화 엔진을 위한 순수 모니터링 시스템. Prometheus, InfluxDB 등 무거운 시계열 DB 없이, C → SQLite → Grafana 직결 구조로 실시간 관제를 구현합니다.

---

## 핵심 특징

- ✅ **Zero Middleware**: 데이터 수집 에이전트 불필요
- ✅ **Single File Storage**: 단일 SQLite 파일로 모든 데이터 관리
- ✅ **Local First**: 인터넷 연결 없이 완전히 동작
- ✅ **Native Performance**: C의 성능을 100% 활용
- ✅ **Easy Backup**: `cp` 명령어로 백업 완료

---

## 아키텍처

```
AEON (C)  ─┬─> SQLite (.db)  ──> Grafana (시각화)
           │     └─ evolution_stats
           │     └─ error_logs
           │
           └─> 파일 I/O (네트워크 0%)
```

### 구성 요소
| 컴포넌트 | 역할 | 파일 크기 |
|---------|------|----------|
| `aeon_monitor.c` | SQLite 연동 C 라이브러리 | ~8KB |
| `aeon_evolution.db` | 진화 데이터 저장소 | ~100KB/일 |
| Grafana | 실시간 대시보드 | Docker 이미지 |

---

## 빠른 시작

### 1. 환경 요구사항
```bash
# Ubuntu/Debian
sudo apt install -y gcc sqlite3 libsqlite3-dev docker.io docker-compose

# 또는 Arch/Manjaro
sudo pacman -S gcc sqlite docker docker-compose
```

### 2. Grafana 실행
```bash
cd aeon-pure-monitoring
docker compose up -d

# 포트 매니저 자동 할당: 40005
# 접속: http://localhost:40005
# ID: master_planner
# PW: aeon_sovereign
```

### 3. C 라이브러리 빌드 및 테스트
```bash
cd src
make test

# 출력 예시:
# [AEON Monitor] DB 초기화 완료: ../data/aeon_evolution.db
# [AEON Stats] Gen:0 | Fitness:0.5000 | Workers:4 | Errors:0 | Status:running
# [AEON Stats] Gen:1 | Fitness:0.5500 | Workers:4 | Errors:0 | Status:running
# ...
```

### 4. Grafana 대시보드 설정
1. **Data Source 추가**
   - Configuration → Data Sources → Add SQLite
   - Path: `/var/lib/grafana/aeon_data/aeon_evolution.db`

2. **Dashboard Import**
   - Dashboards → Import
   - `grafana/dashboards/aeon-evolution.json` 업로드

---

## C 코드 통합

### AEON 프로그램에 삽입
```c
#include "aeon_monitor.h"

int main() {
    const char *db_path = "./data/aeon_evolution.db";

    // 초기화 (프로그램 시작 시 1회)
    aeon_monitor_init(db_path);

    // 진화 루프
    for (int gen = 0; gen < 1000; gen++) {
        // ... AEON 진화 로직 ...

        // 통계 기록
        AeonStats stats = {
            .fitness = calculate_fitness(),
            .generation = gen,
            .workers = 4,
            .errors = 0,
            .status = "running"
        };
        aeon_monitor_log(db_path, &stats);

        // 에러 발생 시
        if (error_occurred) {
            aeon_monitor_log_error(db_path, "Worker crashed");
        }
    }

    return 0;
}
```

### 컴파일
```bash
gcc -o aeon_main aeon_main.c aeon_monitor.c -lsqlite3
```

---

## SQL 쿼리 예시 (Grafana)

### 1. 적합도 진화 그래프
```sql
SELECT
    timestamp as time,
    fitness
FROM evolution_stats
ORDER BY timestamp ASC
```

### 2. 현재 상태
```sql
SELECT status
FROM evolution_stats
ORDER BY timestamp DESC
LIMIT 1
```

### 3. 에러 통계 (최근 1시간)
```sql
SELECT
    COUNT(*) as error_count
FROM error_logs
WHERE timestamp > strftime('%s', 'now') - 3600
```

### 4. 세대별 평균 적합도
```sql
SELECT
    generation,
    AVG(fitness) as avg_fitness
FROM evolution_stats
GROUP BY generation
ORDER BY generation
```

---

## 데이터 관리

### DB 크기 확인
```bash
ls -lh data/aeon_evolution.db
```

### 수동 정리 (7일 이전 데이터 삭제)
```bash
cd src && make clean && ./aeon_monitor_test
```

### 백업
```bash
# 단순 복사
cp data/aeon_evolution.db backups/aeon_$(date +%Y%m%d).db

# 압축 백업
tar czf aeon_backup_$(date +%Y%m%d).tar.gz data/
```

### DB 직접 조회
```bash
sqlite3 data/aeon_evolution.db

sqlite> SELECT * FROM evolution_stats LIMIT 5;
sqlite> SELECT COUNT(*) FROM error_logs;
sqlite> .quit
```

---

## 성능 특성

| 지표 | 값 |
|-----|-----|
| 기록 속도 | ~50,000 writes/sec (SQLite 기본) |
| 메모리 사용량 | ~1MB (C 라이브러리) |
| 디스크 I/O | ~5KB/write (평균) |
| 지연 시간 | <1ms (로컬 파일) |

---

## 철학 문서

이 프로젝트의 설계 철학은 [`docs/PHILOSOPHY.md`](docs/PHILOSOPHY.md)에서 확인하세요.

- 왜 Prometheus를 쓰지 않는가?
- 왜 네트워크를 타지 않는가?
- 왜 SQLite인가?

---

## 확장 가능성

### c-vite 통합
```c
// 같은 DB에 c-vite 성능 데이터 기록
aeon_monitor_log_cvite(db_path, request_count, latency_ms);
```

### Kim War Room 통합
```bash
# 여러 프로젝트의 DB를 한 Grafana에 연결
docker-compose.yml:
  volumes:
    - ./aeon/data:/var/lib/grafana/aeon_data:ro
    - ./cvite/data:/var/lib/grafana/cvite_data:ro
```

### 알림 설정 (Grafana Alerting)
- 적합도가 24시간 동안 개선되지 않으면 알림
- 에러 로그가 10개 이상 쌓이면 경고

---

## 라이선스

MIT License (자유 사용 가능)

---

## 문의

- **프로젝트**: AEON Evolution Engine
- **철학**: 0% 외부 의존성, 기술 주권
- **컨셉**: Pure C Implementation (순수 C 구현)

---

**"복잡성은 적이다. 단순함은 주권이다."**
— Master Planner
