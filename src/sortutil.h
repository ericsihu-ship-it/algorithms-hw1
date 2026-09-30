/* 정렬 구현끼리만 쓰는 도구 — 구현 전용 헤더.
 *
 * sort.h가 바깥에 보이는 인터페이스라면 이쪽은 구현의 공구함이다. 부르는 쪽
 * (main.c · bench.c · 테스트)은 이 파일을 include하지 않는다.
 *
 * 세 정렬은 원소를 **이 파일의 함수로만** 만진다. 비교는 sortCompare, 옮겨 적기는
 * sortCopy, 교환은 sortSwap. 그래서 비교 · 이동 횟수를 정렬 코드가 아니라 여기
 * 한곳에서 센다. 셋을 같은 규칙으로 세야 표 하나에 올려 견줄 수 있다.
 *
 * 인덱스는 ptrdiff_t(부호 있는 정수)로 받는다. 수업 코드가 int로 `hi = p - 1`을
 * 부르는 모양을 그대로 옮기려면 -1이 표현되어야 하기 때문이다.
 */
#ifndef SORTUTIL_H
#define SORTUTIL_H

#include <stddef.h>
#include <string.h>

#include "sort.h"

typedef struct SortContext {
    char *base;         /* 배열의 첫 바이트 */
    size_t size;        /* 원소 하나의 바이트 수 */
    SortCompare cmp;
    SortStats *stats;   /* 늘 유효하다. 부른 쪽이 NULL을 주면 scratch를 가리킨다 */
    SortStats scratch;
    char *tmp;          /* 교환이 거쳐 가는 원소 한 칸. 교환을 쓰는 정렬만 잡는다 */
    size_t heldBytes;   /* 지금 잡고 있는 작업 공간 */
    size_t depth;       /* 지금 재귀 깊이 */
} SortContext;

/* 정렬할 것이 있으면 1을 돌려준다. n < 2처럼 할 일이 없으면 0이고, 그때는
 * sortClose를 부르지 않는다. 어느 쪽이든 stats는 0으로 초기화된다. */
int sortOpen(SortContext *c, void *base, size_t n, size_t size,
             SortCompare cmp, SortStats *stats);
void sortClose(SortContext *c);

/* 작업 공간을 잡고 놓는다. 잡은 바이트의 최댓값이 extraBytes가 된다. */
void *sortAlloc(SortContext *c, size_t bytes);
void sortRelease(SortContext *c, void *p, size_t bytes);

/* a[i]의 주소 */
static inline char *sortAt(const SortContext *c, ptrdiff_t i) {
    return c->base + i * (ptrdiff_t)c->size;
}

/* 비교 1회 */
static inline int sortCompare(SortContext *c, const void *x, const void *y) {
    c->stats->compares++;
    return c->cmp(x, y);
}

/* 이동 1회: 원소 하나를 src에서 dst로 옮겨 적는다.
 *
 * 크기를 실행 중에야 아는 memcpy는 라이브러리 함수 호출이라, 8바이트짜리 원소
 * 하나를 옮기는 데도 호출 비용이 붙는다. 그 비용이 세 정렬의 시간 차이를
 * 흐리지 않도록, 자주 쓰는 크기(Record 8바이트, int 4바이트)는 상수 크기로
 * 따로 적어 컴파일러가 명령 하나로 바꾸게 한다. 세는 방법은 같다. */
static inline void sortCopy(SortContext *c, void *dst, const void *src) {
    if (c->size == 8) {
        memcpy(dst, src, 8);
    } else if (c->size == 4) {
        memcpy(dst, src, 4);
    } else {
        memcpy(dst, src, c->size);
    }
    c->stats->moves++;
}

/* a[i]와 a[j]를 맞바꾼다. tmp를 거치므로 이동 3회다.
 * 수업 코드의 swap처럼 i == j면 아무것도 하지 않는다(세지도 않는다). */
static inline void sortSwap(SortContext *c, ptrdiff_t i, ptrdiff_t j) {
    if (i == j) {
        return;
    }
    sortCopy(c, c->tmp, sortAt(c, i));
    sortCopy(c, sortAt(c, i), sortAt(c, j));
    sortCopy(c, sortAt(c, j), c->tmp);
}

/* 재귀 한 단계를 들어가고 나온다. 들어갈 때 최대 깊이를 갱신한다. */
static inline void sortEnter(SortContext *c) {
    c->depth++;
    if (c->depth > c->stats->maxDepth) {
        c->stats->maxDepth = c->depth;
    }
}

static inline void sortLeave(SortContext *c) {
    c->depth--;
}

#endif /* SORTUTIL_H */
