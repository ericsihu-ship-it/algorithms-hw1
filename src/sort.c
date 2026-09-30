/* 공통 토대 — 세 정렬이 함께 쓰는 도구와, 그 정렬들을 모아 둔 구현 표.
 *
 * 정렬 알고리즘 자체는 각각 제 파일에 있다:
 *   mergeSort.c · quickSort.c · heapSort.c
 */
#include "sort.h"

#include <stdlib.h>

#include "sortutil.h"

/* --- 작업 문맥 --------------------------------------------------------- */

int sortOpen(SortContext *c, void *base, size_t n, size_t size,
             SortCompare cmp, SortStats *stats) {
    c->stats = (stats != NULL) ? stats : &c->scratch;
    c->stats->compares = 0;
    c->stats->moves = 0;
    c->stats->extraBytes = 0;
    c->stats->maxDepth = 1; /* 재귀가 없어도 정렬 함수 자신이 한 단계다 */

    c->base = (char *)base;
    c->size = size;
    c->cmp = cmp;
    c->tmp = NULL;
    c->heldBytes = 0;
    c->depth = 0;
    return base != NULL && cmp != NULL && size > 0 && n >= 2;
}

void sortClose(SortContext *c) {
    if (c->tmp != NULL) {
        sortRelease(c, c->tmp, c->size);
        c->tmp = NULL;
    }
}

void *sortAlloc(SortContext *c, size_t bytes) {
    void *p = malloc(bytes);
    if (p != NULL) {
        c->heldBytes += bytes;
        if (c->heldBytes > c->stats->extraBytes) {
            c->stats->extraBytes = c->heldBytes;
        }
    }
    return p;
}

void sortRelease(SortContext *c, void *p, size_t bytes) {
    if (p != NULL) {
        free(p);
        c->heldBytes -= bytes;
    }
}

int sortCompareInt(const void *a, const void *b) {
    int x = *(const int *)a;
    int y = *(const int *)b;
    return (x > y) - (x < y); /* x - y는 overflow가 날 수 있어 쓰지 않는다 */
}

/* --- 구현 표 ----------------------------------------------------------- */

/* 정렬을 하나 더 넣으려면 파일을 하나 더 두고 여기에 한 줄 넣는다.
 * main.c도 테스트도 이 표만 훑으므로 그것으로 끝이다.
 * 퀵 정렬의 추가 공간은 재귀 스택이다. 피벗이 고르게 떨어지면 O(log n)이고,
 * 한쪽으로만 떨어지면 O(n)까지 간다(실험 1 · 3의 재귀 깊이 열). */
const SortAlgorithm SORT_ALGORITHMS[] = {
    {"mergeSort", "O(n log n)", "O(n log n)", "O(n)",     1, mergeSort},
    {"quickSort", "O(n log n)", "O(n^2)",     "O(log n)", 0, quickSort},
    {"heapSort",  "O(n log n)", "O(n log n)", "O(1)",     0, heapSort},
};

const size_t SORT_ALGORITHM_COUNT = sizeof(SORT_ALGORITHMS) / sizeof(SORT_ALGORITHMS[0]);
