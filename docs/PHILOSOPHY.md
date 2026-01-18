# AEON Pure Monitoring Philosophy

## 핵심 철학: 0% 외부 의존성

### 왜 순수(Pure) 구현인가?

1. **기술 주권 (Technological Sovereignty)**
   - 외부 서비스(Prometheus, InfluxDB, Elastic 등)에 의존하지 않음
   - 데이터는 항상 로컬 파일 시스템에 존재
   - 인터넷 연결 없이도 완전히 동작

2. **단순성 (Simplicity)**
   - 복잡한 스크래퍼나 에이전트 불필요
   - C 코드가 직접 SQLite 파일에 기록
   - 미들웨어 레이어 0개

3. **성능 (Performance)**
   - 네트워크 홉 없음 (파일 I/O만 사용)
   - 메모리 오버헤드 최소화
   - C의 네이티브 성능 100% 활용

4. **신뢰성 (Reliability)**
   - 단일 실패 지점(Single Point of Failure) 제거
   - DB 파일은 백업/이동이 극도로 쉬움
   - 파일 시스템만 동작하면 OK

---

## 아키텍처 비교

### 전통적 방식 (복잡함)
```
AEON → Exporter → Prometheus → Grafana
        (HTTP)     (Storage)    (Query)
```
- 3개의 추가 프로세스 필요
- 네트워크 통신 2회
- 설정 파일 3개 이상

### 순수 AEON 방식 (단순함)
```
AEON → SQLite → Grafana
     (write)   (read)
```
- 추가 프로세스 1개 (Grafana만)
- 네트워크 통신 0회
- 설정 파일 1개

---

## 데이터 흐름

### 1. AEON (C 프로그램)
```c
AeonStats stats = {
    .fitness = current_fitness,
    .generation = gen_count,
    .workers = 4,
    .status = "running"
};
aeon_monitor_log("./data/aeon.db", &stats);
```

### 2. SQLite (파일)
```
./data/aeon_evolution.db
├── evolution_stats (진화 통계)
└── error_logs (에러 로그)
```

### 3. Grafana (시각화)
```sql
SELECT timestamp, fitness
FROM evolution_stats
ORDER BY timestamp ASC
```

---

## 기술 선택의 이유

### SQLite를 선택한 이유
1. **Zero Configuration**: 설치나 서버 설정 불필요
2. **ACID 보장**: 트랜잭션 안정성 확보
3. **Embedded DB**: 별도 프로세스 필요 없음
4. **단일 파일**: 백업이 `cp` 명령어 하나로 끝남
5. **표준 SQL**: 복잡한 쿼리도 가능

### Grafana를 선택한 이유
1. **SQLite 플러그인 공식 지원**: frser-sqlite-datasource
2. **오픈소스**: 소스코드 검증 가능
3. **강력한 시각화**: 프로페셔널한 대시보드
4. **경량 컨테이너**: Docker로 쉽게 배포

---

## 운영 원칙

### 1. 데이터 보관 정책
- 기본: 7일 자동 삭제
- 중요 이벤트: 별도 테이블에 영구 보관
- DB 크기 제한: 100MB 넘으면 경고

### 2. 백업 전략
```bash
# 실시간 백업 (cron)
0 * * * * cp /data/aeon.db /backups/aeon_$(date +\%Y\%m\%d_\%H).db
```

### 3. 장애 대응
- Grafana 다운: C 프로그램은 계속 동작 (DB 기록은 계속됨)
- DB 손상: 자동으로 새 DB 생성
- 디스크 부족: 오래된 데이터 자동 삭제

---

## 미래 확장성

이 구조는 다른 AEON 컴포넌트에도 재사용 가능합니다:

1. **c-vite 성능 모니터링**
   - 같은 DB의 `c_vite_stats` 테이블 사용
   - 한 화면에서 AEON + c-vite 통합 관제

2. **Kim War Room 통합**
   - 여러 프로젝트의 SQLite DB를 한 Grafana에 연결
   - 통합 대시보드 구성

3. **알림 시스템**
   - Grafana Alerting 기능 활용
   - SQLite 트리거로 자동 에러 감지

---

## 결론: 순수함의 가치

"순수하다"는 것은 불필요한 복잡성을 제거하고 본질에 집중하는 것입니다.
AEON의 진화 데이터를 관제하는 데, 무거운 인프라는 필요하지 않습니다.
**C 프로그램 → SQLite 파일 → Grafana**
이 세 가지만으로 충분합니다.

---

**Master Planner의 약속:**
이 시스템은 인터넷이 끊겨도, 외부 서비스가 망가져도, 항상 동작합니다.
당신의 데이터는 당신의 디스크에만 존재합니다.
이것이 기술 주권입니다.
