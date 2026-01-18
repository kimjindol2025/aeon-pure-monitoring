# AEON Pure Monitoring

**Grafana-Style Multi-Dashboard System | 0% External Dependencies**

순수 C로 구현된 모니터링 대시보드 시스템. 외부 라이브러리 없이 SQLite + HTTP Server + Pure HTML/JS로 동작.

## 특징

- **100% 순수 C 구현**: Node.js, Python 없이 단일 바이너리
- **0% 외부 의존성**: npm, pip 설치 불필요
- **서버사이드 차트**: SVG 차트를 C에서 직접 생성
- **동적 위젯 시스템**: 6가지 위젯 타입 (stat, line-chart, bar-chart, table, gauge, log)
- **멀티 대시보드**: 무제한 대시보드 생성 가능
- **RESTful API**: JSON 기반 CRUD 인터페이스

## 대시보드 구성 (17개 위젯)

### Row 0: 주요 통계
- CPU Usage, Memory Usage, Active Workers, Total Errors

### Row 1: 시스템 차트
- CPU/Memory 시계열 차트

### Row 2: 네트워크 & 디스크
- Network Traffic, Disk Usage, Active Connections

### Row 3: 진화 엔진
- Fitness Evolution, Best Fitness, Generation

### Row 4: API 성능
- API Requests 테이블, Avg Latency, Error Rate

### Row 5: 워커 & 에러
- Worker Status 테이블, Error Logs

## 빌드 & 실행

```bash
cd src
make
./aeon_api_server_v2
```

서버: http://localhost:40005

## 배포

https://multi-monitor.dclub.kr

## 기술 스택

- Pure C (POSIX sockets)
- SQLite3 C API
- Vanilla JavaScript (ES6)
- Server-side SVG generation

## 라이센스

MIT License
