# 기여 가이드

AEON Pure Monitoring에 기여해주셔서 감사합니다! 이 문서는 프로젝트에 기여하는 방법을 설명합니다.

## 📋 목차

- [행동 강령](#-행동-강령)
- [시작하기](#-시작하기)
- [개발 환경 설정](#-개발-환경-설정)
- [코드 스타일](#-코드-스타일)
- [테스트](#-테스트)
- [커밋 메시지](#-커밋-메시지)
- [PR 제출](#-pr-제출)

## 🤝 행동 강령

이 프로젝트는 모두에게 열려 있습니다. 다음을 준수해주세요:

- 존중하고 포용적인 환경 유지
- 건설적인 피드백 제공
- 다른 관점 존중
- 정중한 커뮤니케이션

## 🚀 시작하기

### 1. Fork & Clone

```bash
# GitHub에서 Fork
https://github.com/yourusername/aeon-pure-monitoring/fork

# 로컬 복제
git clone https://github.com/yourusername/aeon-pure-monitoring.git
cd aeon-pure-monitoring

# Upstream 추가
git remote add upstream https://github.com/original/aeon-pure-monitoring.git
```

### 2. 브랜치 생성

```bash
# 최신 코드로 업데이트
git fetch upstream
git rebase upstream/master

# 기능 브랜치 생성
git checkout -b feature/my-awesome-feature
```

### 3. 개발 & 테스트

```bash
# 변경사항 작성
# (아래 코드 스타일 섹션 참고)

# 빌드
cd src
make clean && make

# 테스트 실행
./test_runner

# 코드 검사
make lint
```

## 💻 개발 환경 설정

### 필수 도구

```bash
# Ubuntu/Debian
sudo apt-get install \
  build-essential \
  git \
  sqlite3 \
  libsqlite3-dev \
  valgrind \
  cppcheck

# macOS
brew install sqlite3 valgrind

# 선택사항: 정적 분석
sudo apt-get install clang-tools
```

### 빌드 및 실행

```bash
cd src

# 개발 빌드 (디버그 정보 포함)
make DEBUG=1

# 최적화 빌드
make RELEASE=1

# 메모리 검사 (Valgrind)
valgrind --leak-check=full ./aeon_api_server_v2

# 정적 분석
cppcheck --enable=all .
```

## 🎯 코드 스타일

### C 코드 스타일 (K&R 변형)

```c
// 함수 정의
int function_name(int param1, const char *param2) {
    // 로컬 변수
    int local_var = 0;

    // 구현
    if (param1 > 0) {
        local_var = param1 * 2;
    }

    return local_var;
}

// 구조체
typedef struct {
    int id;
    char name[256];
    double value;
} MyStruct;

// 상수
#define MAX_BUFFER_SIZE 1024
#define MIN_PORT 1024

// 에러 처리
if (ptr == NULL) {
    fprintf(stderr, "Error: Memory allocation failed\n");
    return -1;
}
```

### 네이밍 컨벤션

```c
// 함수: snake_case
int get_user_by_id(int user_id);
void process_metrics_data(void);

// 상수: UPPER_SNAKE_CASE
#define MAX_CONNECTIONS 1000
#define DEFAULT_PORT 40005

// 타입: PascalCase (typedef)
typedef struct UserData UserData;
typedef struct DatabaseConfig DatabaseConfig;

// 변수: snake_case
int user_count = 0;
char buffer[256];
double response_time_ms;
```

### 주석 및 문서화

```c
/**
 * 메트릭 데이터를 데이터베이스에 저장합니다.
 *
 * @param metric_name 메트릭 이름
 * @param value 메트릭 값
 * @param timestamp 타임스탬프 (Unix epoch)
 *
 * @return 성공 시 0, 실패 시 -1
 */
int store_metric(const char *metric_name, double value, time_t timestamp);

// 간단한 주석
int count = 0;  // 메트릭 카운트
```

### 파일 구조

```c
/*
 * file: metrics.c
 * description: 메트릭 수집 및 저장 로직
 * author: Kim Team
 * license: MIT
 */

#include <stdio.h>
#include <stdlib.h>
#include "metrics.h"
#include "database.h"

// 상수 정의
#define METRIC_BUFFER_SIZE 1024

// 정적 함수 (내부용)
static int validate_metric_value(double value);

// 공개 함수 (API)
int store_metric(const char *metric_name, double value, time_t timestamp) {
    // 구현...
}
```

## ✅ 테스트

### 테스트 작성

```bash
# 테스트 파일: src/tests/test_metrics.c
# 테스트 실행: make test
# 테스트 커버리지: make coverage
```

### 테스트 체크리스트

- [ ] 단위 테스트 작성 (새 함수)
- [ ] 통합 테스트 추가 (기능)
- [ ] 엣지 케이스 테스트
- [ ] 메모리 누수 테스트 (valgrind)
- [ ] 정적 분석 통과 (cppcheck)

## 📝 커밋 메시지

### 형식

```
<type>: <subject>

<body>

<footer>
```

### Type

- `feat`: 새 기능
- `fix`: 버그 수정
- `docs`: 문서화
- `style`: 코드 포맷 (함수 기능 변화 없음)
- `refactor`: 코드 리팩토링
- `perf`: 성능 개선
- `test`: 테스트 추가
- `chore`: 빌드, 의존성 등

### 예시

```
feat: Add widget type 'pie-chart'

Implement pie chart widget support with the following features:
- SVG rendering (C-side)
- Interactive legend
- Custom color scheme
- Responsive design

Fixes #123
```

## 🔄 PR 제출

### PR 체크리스트

```markdown
## 설명
<!-- 변경사항 설명 -->

## 타입
- [ ] 버그 수정
- [ ] 새 기능
- [ ] 문서 변경
- [ ] 리팩토링
- [ ] 성능 개선

## 테스트
- [ ] 단위 테스트 작성 ✓
- [ ] 통합 테스트 통과 ✓
- [ ] 메모리 누수 없음 (valgrind) ✓
- [ ] 정적 분석 통과 (cppcheck) ✓

## 체크리스트
- [ ] 코드 리뷰 완료
- [ ] 변경사항 테스트됨
- [ ] 문서 업데이트됨
- [ ] 커밋 메시지 명확함

## 스크린샷 (해당하면)
<!-- 대시보드 UI 변경 시 스크린샷 -->
```

### PR 리뷰 과정

1. **자동 검사**: CI/CD 파이프라인 실행
   - 빌드 성공 확인
   - 테스트 통과 확인
   - 정적 분석 통과

2. **코드 리뷰**: 2명 이상 승인
   - 코드 품질
   - 테스트 커버리지
   - 문서화

3. **병합**: 모든 검사 통과 후 병합

## 🐛 버그 리포트

### Issue 제출 전

- 최신 버전 사용 확인
- 동일 이슈 검색
- 재현 방법 작성

### Issue 양식

```markdown
## 설명
<!-- 버그 설명 -->

## 재현 방법
1. ...
2. ...
3. ...

## 예상 동작
<!-- 어떻게 동작해야 하는가 -->

## 실제 동작
<!-- 실제 동작 -->

## 환경
- OS: Linux/macOS/WSL
- GCC 버전: 9.0+
- 기타: ...

## 로그
<!-- 에러 메시지, 스택 트레이스 -->
```

## ⭐ 기여 아이디어

### 우선 기여 항목

- [ ] 버그 수정 (Issue에서 `good-first-issue` 찾기)
- [ ] 문서 개선
- [ ] 테스트 추가
- [ ] 성능 최적화
- [ ] 새 위젯 타입 추가

### 고급 기여

- [ ] PostgreSQL 백엔드 지원
- [ ] HTTPS/TLS 통합
- [ ] JWT 인증 추가
- [ ] Kubernetes 통합
- [ ] 대시보드 템플릿 추가

## 📚 참고 자료

- [AEON Architecture](./docs/ARCHITECTURE.md)
- [API Documentation](./docs/API.md)
- [Code Review Guidelines](./docs/CODE_REVIEW.md)
- [Performance Tips](./docs/PERFORMANCE.md)

## 💬 질문 & 토론

- **GitHub Discussions**: https://github.com/yourusername/aeon-pure-monitoring/discussions
- **Issues**: https://github.com/yourusername/aeon-pure-monitoring/issues
- **Email**: kim@example.com

## 🎓 학습 리소스

- [C99 Standard](https://en.wikipedia.org/wiki/C99)
- [SQLite C API](https://www.sqlite.org/c3ref/intro.html)
- [POSIX Sockets](https://en.wikipedia.org/wiki/Berkeley_sockets)
- [HTTP Protocol](https://tools.ietf.org/html/rfc7231)

---

**감사합니다! 여러분의 기여로 AEON을 더 좋게 만들어 가겠습니다.** 🚀
