CC = gcc
CFLAGS = -Wall -Wextra -O2 -std=c11
LIBS = -lsqlite3 -lpthread

TARGET = aeon_monitor_test
SOURCES = aeon_monitor.c
OBJECTS = $(SOURCES:.c=.o)
HEADERS = aeon_monitor.h

# 기본 타겟
all: $(TARGET)

# 테스트 실행 파일 빌드 (EXAMPLE 매크로 포함)
$(TARGET): $(SOURCES) $(HEADERS)
	$(CC) $(CFLAGS) -DAEON_MONITOR_EXAMPLE -o $@ $(SOURCES) $(LIBS)

# 오브젝트 파일 생성 (라이브러리용, EXAMPLE 없음)
%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

# 라이브러리 생성 (다른 프로젝트에 링크용)
libaeon_monitor.a: aeon_monitor.o
	ar rcs $@ $^

# 테스트 실행
test: $(TARGET)
	@echo "=== AEON Monitor 테스트 실행 ==="
	mkdir -p ../data
	./$(TARGET)
	@echo "=== DB 내용 확인 ==="
	sqlite3 ../data/aeon_evolution.db "SELECT * FROM evolution_stats LIMIT 5;"

# 정리
clean:
	rm -f $(OBJECTS) $(TARGET) libaeon_monitor.a
	rm -f ../data/*.db

# 설치 (시스템 전역)
install: libaeon_monitor.a
	cp aeon_monitor.h /usr/local/include/
	cp libaeon_monitor.a /usr/local/lib/
	@echo "라이브러리 설치 완료 (/usr/local/)"

.PHONY: all test clean install
