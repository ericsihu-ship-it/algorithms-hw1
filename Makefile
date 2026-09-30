# 빌드와 테스트를 한 단어로 돌리기 위한 Makefile.
# 컨테이너 안에서 실행한다 (docker compose exec lab bash).
#
#   make run       실험 1~4를 표로 찍는다 (수십 초 걸린다)
#   make test      유닛 테스트
#   make charts    실험을 CSV로 받아 report/ 아래에 그래프(SVG)를 다시 그린다
#   make sanitize  ASan · UBSan(메모리 · 미정의 동작 검사기)을 붙여 테스트
#   make debug     디버그 심볼을 넣어 빌드 (VS Code의 F5가 쓴다)
#   make clean     빌드 산출물 정리
#
# 실행 파일은 `*.out`으로 만든다. .gitignore가 그것만 걸러낸다.

CC ?= gcc
CFLAGS ?= -std=c17 -Wall -Wextra -O2
# 디버그 빌드: 최적화를 끄고 심볼을 남긴다 (VS Code의 F5가 이 결과물을 쓴다).
DEBUGFLAGS ?= -std=c17 -Wall -Wextra -g -O0
SANFLAGS ?= -std=c17 -Wall -Wextra -g -O1 -fno-omit-frame-pointer \
            -fsanitize=address,undefined -fno-sanitize-recover=all

# main.c를 뺀 구현 전부. 실험 프로그램과 테스트가 함께 링크한다.
LIB_SRC = $(filter-out src/main.c,$(wildcard src/*.c))
HEADERS = $(wildcard src/*.h)

.PHONY: all run test charts sanitize debug clean

all: test

run: src/main.out
	@./src/main.out

test: tests/test_sort.out
	@./tests/test_sort.out

# 그래프는 표준 모듈만 쓰는 tools/charts.py가 SVG로 직접 그린다.
charts: src/main.out
	@python3 tools/charts.py

sanitize: tests/test_sort.san.out
	@./tests/test_sort.san.out

debug: src/main.debug.out

# 실험 프로그램과 테스트는 헤더나 옆 파일만 바뀌어도 다시 빌드되도록
# 의존 파일을 전부 적는다. 아래 패턴 규칙보다 이 명시 규칙이 우선한다.
src/main.out: src/main.c $(LIB_SRC) $(HEADERS)
	$(CC) $(CFLAGS) -Isrc -o $@ src/main.c $(LIB_SRC)

src/main.debug.out: src/main.c $(LIB_SRC) $(HEADERS)
	$(CC) $(DEBUGFLAGS) -Isrc -o $@ src/main.c $(LIB_SRC)

tests/test_sort.out: tests/test_sort.c $(LIB_SRC) $(HEADERS)
	$(CC) $(CFLAGS) -Isrc -o $@ tests/test_sort.c $(LIB_SRC)

tests/test_sort.san.out: tests/test_sort.c $(LIB_SRC) $(HEADERS)
	$(CC) $(SANFLAGS) -Isrc -o $@ tests/test_sort.c $(LIB_SRC)

# 파일 하나를 그 자리에서 빌드한다 (template 그대로). 같은 폴더의 .c를 함께
# 링크하므로 **한 폴더에 main은 하나만** 둔다.
#   %.out        실행용 (Code Runner의 ▶ 버튼이 이 규칙을 부른다)
#   %.debug.out  디버그용 (VS Code의 "C 디버그 (현재 파일)"이 부른다)
%.out: %.c
	$(CC) $(CFLAGS) -I$(@D) -o $@ $(wildcard $(@D)/*.c)

%.debug.out: %.c
	$(CC) $(DEBUGFLAGS) -I$(@D) -o $@ $(wildcard $(@D)/*.c)

clean:
	rm -f src/*.out tests/*.out
