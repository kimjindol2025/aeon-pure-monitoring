# 포트 매니저 통합

## 등록 정보

- **서버 ID**: 4
- **할당 포트**: 40005
- **서버명**: aeon-grafana
- **상태**: RUNNING
- **태그**: monitoring, grafana, aeon
- **명령어**: `/tmp/start_grafana.sh {port}`

## 접속

```bash
# Grafana 대시보드
http://localhost:40005

# 로그인
ID: master_planner
PW: aeon_sovereign
```

## 포트 매니저 API

### 서버 상태 확인
```bash
curl -s http://localhost:45000/api/servers | jq '.servers[] | select(.name=="aeon-grafana")'
```

### 서버 중지
```bash
curl -X POST http://localhost:45000/api/servers/4/stop
```

### 서버 재시작
```bash
docker compose down
docker compose up -d
```

## 통합 이점

1. **자동 포트 할당**: 충돌 없이 안전한 포트 사용
2. **중앙 관리**: 포트 매니저 웹 UI에서 모니터링
3. **표준화**: 모든 서비스가 40000~49999 범위 사용
4. **충돌 방지**: DB + OS 수준 포트 검증

## 래퍼 스크립트

포트 매니저 연동을 위한 스크립트: `/tmp/start_grafana.sh`

```bash
#!/bin/bash
PORT=${1:-40005}
cd /home/kimjin/Desktop/kim/aeon-pure-monitoring
docker compose up -d
echo "Grafana started on port $PORT"
```

포트는 docker-compose.yml에 하드코딩되어 있으므로, 스크립트는 단순히 컨테이너를 시작합니다.
