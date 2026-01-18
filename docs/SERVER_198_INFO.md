# 198 서버 접속 및 모니터링 정보

## 서버 정보

| 항목 | 값 |
|------|-----|
| **IP** | 192.168.45.198 |
| **호스트명** | kim-ADL-N |
| **사용자** | kim |
| **SSH 포트** | 22 |
| **용도** | AI Gateway 실험장 (완전 자율 AI) |
| **자율 레벨** | L3 (Phase A 완료) |

## SSH 접속

### 내부 네트워크
```bash
ssh kim@192.168.45.198

# 또는 config 사용
ssh 198
```

### SSH Config 설정
```
Host 198
    HostName 192.168.45.198
    User kim
    ServerAliveInterval 60
    ServerAliveCountMax 3
    TCPKeepAlive yes
```

## AI Gateway 상태

### 프로세스
```bash
ssh 198 "ps aux | grep gateway"
# PID: 878
# 실행: /usr/bin/python3 /home/kim/ai-gateway-198/gateway.py
```

### 데이터 위치
```
/home/kim/ai-gateway-198/
├── AI_STATUS_SUMMARY.json       # 전체 상태 요약
├── STATE_20260118.json          # 일일 상태
├── LEARNING_STATUS_20260118.md  # 학습 현황
├── gateway.py                   # 메인 Gateway
├── policy.yaml                  # 정책 파일
├── logs/                        # 로그 디렉토리
│   ├── gateway.log
│   ├── ralph_agent.log
│   └── watchdog.log
└── state/
    └── heartbeat.json           # PID 정보
```

## 모니터링 수집

### gateway_collector.py 동작
```python
# 5초마다 실행
1. SSH로 AI_STATUS_SUMMARY.json 가져오기
2. JSON 파싱
3. SQLite에 저장 (gateway_stats 테이블)
4. API 서버가 /api/stats로 제공
```

### 수집 데이터
- **Server**: 192.168.45.198
- **PID**: 878 (gateway.py)
- **Phase**: Phase A - Ralph Loop
- **Status**: Active Learning
- **Uptime**: 2.82 days
- **Total Commands**: 21
- **Success Rate**: 90.5%
- **Block Rate**: 9.5%
- **Memory Usage**: 6 MB
- **Security Level**: L3

## 현재 학습 상태

### 학습 완료 패턴
**Safe Commands** (11개):
- echo, date, whoami, uptime, df, free
- systemctl, du, ls, tail, wc

**Blocked Commands** (2개):
- rm -rf
- reboot

### 성과 지표
- Task Success Rate: 100%
- Block Accuracy: 100%
- System Uptime: 100%
- Average Execution Time: 12.4ms

## 접속 체크 스크립트

```bash
#!/bin/bash
# 198 서버 상태 확인

echo "=== 198 서버 접속 체크 ==="

# 1. SSH 접속 테스트
ssh -o ConnectTimeout=5 kim@192.168.45.198 "echo 'SSH OK'" 2>&1 || echo "SSH 실패"

# 2. Gateway 프로세스 확인
ssh 198 "ps aux | grep -E 'gateway\.py' | grep -v grep" 2>&1

# 3. 업타임 확인
ssh 198 "uptime" 2>&1

# 4. AI 상태 파일 확인
ssh 198 "cat /home/kim/ai-gateway-198/AI_STATUS_SUMMARY.json | python3 -m json.tool 2>/dev/null | head -20" 2>&1

echo "=== 체크 완료 ==="
```

## 문제 해결

### SSH 접속 안 됨
```bash
# 1. 네트워크 확인
ping 192.168.45.198

# 2. SSH 포트 확인
nc -zv 192.168.45.198 22

# 3. SSH 키 확인
ssh-add -l
```

### Gateway 프로세스 죽음
```bash
# 1. 프로세스 확인
ssh 198 "ps aux | grep gateway"

# 2. 로그 확인
ssh 198 "tail -50 /home/kim/ai-gateway-198/logs/gateway.log"

# 3. 재시작 (수동)
ssh 198 "cd /home/kim/ai-gateway-198 && nohup python3 gateway.py > /dev/null 2>&1 &"
```

### 데이터 수집 안 됨
```bash
# 1. gateway_collector.py 프로세스 확인
ps aux | grep gateway_collector

# 2. 로그 확인
tail -f /tmp/gateway_collector.log

# 3. 수동 테스트
cd /home/kimjin/Desktop/kim/aeon-pure-monitoring/src
python3 gateway_collector.py
```

## 모니터링 대시보드

**URL**: http://localhost:40005

**Gateway 섹션**:
- Server: 192.168.45.198 표시
- Phase: Phase A - Ralph Loop
- Success Rate: 90.5%
- Total Commands: 21
- Uptime: 2.82d
- Level: L3

## Gogs 저장소

- **198 Gateway**: https://gogs.ai-empire.kr/kim/ai-gateway-198
- **모니터링**: https://gogs.dclub.kr/kim/aeon-pure-monitoring

---

**마지막 업데이트**: 2026-01-18 22:25 KST
**Uptime**: 2 days, 20:25
**Load Average**: 0.04, 0.08, 0.10
