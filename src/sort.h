/* 과제 1 — 정렬 비교: 병합 · 퀵 · 힙.
 *
 * 세 정렬을 같은 모양의 함수로 만들고, 그 함수들을 담은 표 하나로 부른다.
 * 부르는 쪽(main.c · bench.c · 테스트)은 정렬 이름을 적지 않고 표만 훑는다.
 * C에는 interface가 없으므로 함수 포인터를 담은 구조체가 그 자리를 맡는다.
 *
 * 정렬은 원소의 타입을 모른다. qsort처럼 (시작 주소, 개수, 원소 크기, 비교
 * 함수)를 받는다. 그래서 (key, order) 구조체를 넣어 안정성을 직접 잴 수 있다.
 */
#ifndef SORT_H
#define SORT_H

#include <stddef.h>

/* qsort와 같은 비교 규약: a < b면 음수, 같으면 0, a > b면 양수. */
typedef int (*SortCompare)(const void *a, const void *b);

/* 정렬 한 번에서 센 값. 입력이 같으면 어느 기계에서 돌려도 같은 값이 나온다.
 * 시간은 기계마다 달라서 여기 두지 않고 바깥(bench.c)에서 잰다. */
typedef struct SortStats {
    size_t compares;   /* 비교 함수를 부른 횟수 */
    size_t moves;      /* 원소 하나를 옮겨 적은 횟수. 교환 한 번은 3 */
    size_t extraBytes; /* 입력 배열 밖에 잡은 작업 공간의 최댓값(바이트). 스택은 빠진다 */
    size_t maxDepth;   /* 원소 둘 이상을 맡은 재귀 호출이 쌓인 최대 깊이. 재귀가 없으면 1 */
} SortStats;

/* base[0..n-1]을 제자리에서 오름차순으로 정렬한다. 원소 하나는 size 바이트다.
 * stats가 NULL이면 세지 않는다. */
typedef void (*SortFunction)(void *base, size_t n, size_t size,
                             SortCompare cmp, SortStats *stats);

/* 정렬 하나. 이 구조체가 이 과제의 "인터페이스"다. */
typedef struct SortAlgorithm {
    const char *name;
    const char *averageTime; /* 평균 시간복잡도 */
    const char *worstTime;   /* 최악 시간복잡도 */
    const char *extraSpace;  /* 추가 공간 (재귀 스택 포함) */
    int stable;              /* 안정 정렬이라고 "주장"하는 값. 테스트가 실측과 맞춰 본다 */
    SortFunction sort;
} SortAlgorithm;

/* --- 세 정렬 ---------------------------------------------------------- */

void mergeSort(void *base, size_t n, size_t size, SortCompare cmp, SortStats *stats);
void quickSort(void *base, size_t n, size_t size, SortCompare cmp, SortStats *stats);
void heapSort(void *base, size_t n, size_t size, SortCompare cmp, SortStats *stats);

/* 힙 정렬의 1단계만 따로 부른다: base[0..n-1]을 최대 힙으로 만든다.
 * 실험 4가 "힙 만들기는 O(n)"을 이것으로 확인한다. */
void buildMaxHeap(void *base, size_t n, size_t size, SortCompare cmp, SortStats *stats);

/* --- 퀵 정렬의 두 매개변수 ---------------------------------------------- */

typedef enum QuickPivot {
    QUICK_PIVOT_FIRST,  /* 구간의 첫 원소. 수업 코드 그대로 */
    QUICK_PIVOT_RANDOM, /* 구간 안에서 무작위. 주제 04의 옵션 1 */
    QUICK_PIVOT_MEDIAN3 /* 앞 · 가운데 · 뒤 셋 중 가운데 값. 주제 04의 옵션 2 */
} QuickPivot;

typedef enum QuickPartition {
    QUICK_PARTITION_TWO_WAY,  /* 수업의 파티션: [피벗보다 작은 것 | 나머지] */
    QUICK_PARTITION_THREE_WAY /* [작은 것 | 같은 것 | 큰 것]. 같은 값은 다시 보지 않는다 */
} QuickPartition;

void quickSortWith(void *base, size_t n, size_t size, SortCompare cmp,
                   SortStats *stats, QuickPivot pivot, QuickPartition partition);

/* --- 구현 표 ---------------------------------------------------------- */

/* 본 비교의 세 정렬. quickSort는 수업의 2-way 파티션에 랜덤 피벗을 쓴다. */
extern const SortAlgorithm SORT_ALGORITHMS[];
extern const size_t SORT_ALGORITHM_COUNT;

/* 실험 3: 퀵 정렬의 피벗 세 가지 × 파티션 두 가지 = 여섯 변형. */
extern const SortAlgorithm QUICK_VARIANTS[];
extern const size_t QUICK_VARIANT_COUNT;

int sortCompareInt(const void *a, const void *b); /* int 배열용 비교 함수 */

#endif /* SORT_H */
