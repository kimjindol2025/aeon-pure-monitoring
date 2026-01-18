CC = gcc
CFLAGS = -Wall -Wextra -O2 -std=c11
LIBS = -lsqlite3 -lpthread
GATEWAY_LIBS = -lsqlite3 -lcurl -ljson-c

TARGET = aeon_monitor_test
API_SERVER = aeon_api_server
API_SERVER_V2 = aeon_api_server_v2
GATEWAY_MONITOR = gateway_monitor
SOURCES = aeon_monitor.c
OBJECTS = $(SOURCES:.c=.o)
DASHBOARD_OBJECTS = dashboard_manager.o
HEADERS = aeon_monitor.h dashboard_manager.h

# 기본 타겟
all: $(TARGET) $(API_SERVER) $(API_SERVER_V2)

# 테스트 실행 파일 빌드 (EXAMPLE 매크로 포함)
$(TARGET): $(SOURCES) $(HEADERS)
	$(CC) $(CFLAGS) -DAEON_MONITOR_EXAMPLE -o $@ $(SOURCES) $(LIBS)

# 오브젝트 파일 생성 (라이브러리용, EXAMPLE 없음)
%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

# 라이브러리 생성 (다른 프로젝트에 링크용)
libaeon_monitor.a: aeon_monitor.o
	ar rcs $@ $^

# API 서버 빌드 (순수 C 구현)
$(API_SERVER): api_server.c aeon_monitor.o
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

# API 서버 V2 빌드 (멀티 대시보드)
$(API_SERVER_V2): api_server_v2.c aeon_monitor.o dashboard_manager.o
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

# Gateway 모니터 빌드
$(GATEWAY_MONITOR): gateway_monitor.c
	$(CC) $(CFLAGS) -DGATEWAY_MONITOR_EXAMPLE -o $@ $^ $(GATEWAY_LIBS)

# 테스트 실행
test: $(TARGET)
	@echo "=== AEON Monitor 테스트 실행 ==="
	mkdir -p ../data
	./$(TARGET)
	@echo "=== DB 내용 확인 ==="
	sqlite3 ../data/aeon_evolution.db "SELECT * FROM evolution_stats LIMIT 5;"

# API 서버 실행 (레거시)
run-api: $(API_SERVER)
	@echo "=== AEON API Server 시작 ==="
	@echo "포트: 40005"
	@echo "URL: http://localhost:40005"
	./$(API_SERVER)

# API 서버 V2 실행 (멀티 대시보드)
run-api-v2: $(API_SERVER_V2)
	@echo "=== AEON Multi-Dashboard API Server 시작 ==="
	@echo "포트: 40005"
	@echo "URL: http://localhost:40005"
	./$(API_SERVER_V2)

# Gateway 모니터 실행
run-gateway: $(GATEWAY_MONITOR)
	@echo "=== Gateway Monitor 시작 (198 서버) ==="
	./$(GATEWAY_MONITOR)

# 정리
clean:
	rm -f $(OBJECTS) $(DASHBOARD_OBJECTS) $(TARGET) $(API_SERVER) $(API_SERVER_V2) $(GATEWAY_MONITOR) libaeon_monitor.a
	rm -f ../data/*.db

# 설치 (시스템 전역)
install: libaeon_monitor.a
	cp aeon_monitor.h /usr/local/include/
	cp libaeon_monitor.a /usr/local/lib/
	@echo "라이브러리 설치 완료 (/usr/local/)"

.PHONY: all test clean install
