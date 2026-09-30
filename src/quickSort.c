/* 퀵 정렬 — 피벗을 제자리에 놓고, 양쪽을 각각 정렬한다 (주제 04).
 *
 * quickSort()가 본 비교에 쓰는 판이다. 수업의 파티션(algorithm-code의
 * quickSort.c)을 그대로 두고 피벗만 랜덤으로 고른다. 주제 04가 정렬된 입력의
 * 해독제로 내놓은 "옵션 1"이다.
 *
 * quickSortWith()는 피벗과 파티션을 바꿔 끼운다. 실험 3이 이것으로 잰다.
 *
 *   피벗    first   구간의 첫 원소 (수업 코드)
 *           random  구간 안에서 무작위 (옵션 1)
 *           median3 앞 · 가운데 · 뒤 셋 중 가운데 값 (옵션 2)
 *   파티션  2-way   [피벗보다 작은 것 | 나머지] (수업 코드)
 *           3-way   [작은 것 | 같은 것 | 큰 것]
 *
 * 수업의 2-way 파티션은 피벗과 **같은 값**을 전부 오른쪽으로 보낸다. 같은 값이
 * 많으면 왼쪽이 거의 비고, 트리가 외길이 되어 O(n^2)로 떨어진다. 이것은 피벗을
 * 어떻게 골라도 피할 수 없다. 값이 다 같은 구간에서는 어느 원소를 골라도 같은
 * 값이기 때문이다. 3-way 파티션은 같은 값을 가운데에 모아 두고 다시 보지 않는다.
 *
 * 어느 판이든 멀리 떨어진 원소를 맞바꾸므로 불안정하다.
 */
#include "sort.h"

#include <stdint.h>

#include "minstd.h"
#include "sortutil.h"

/* 피벗용 난수의 시드. 고정해 두면 같은 입력에서 비교 횟수까지 똑같이 재현된다. */
static const uint32_t PIVOT_SEED = 1;

/* 정렬 한 번 동안 들고 다니는 것. */
typedef struct QuickRun {
    SortContext *c;
    QuickPivot pivot;
    QuickPartition partition;
    Minstd rng;      /* random 피벗용 */
    char *pivotCopy; /* 3-way 파티션이 피벗 값을 들고 있는 한 칸 */
} QuickRun;

/* --- 피벗 고르기 -------------------------------------------------------- */

/* a[i], a[j], a[k] 중 가운데 값의 위치. 비교 2~3회. */
static ptrdiff_t medianOfThree(SortContext *c, ptrdiff_t i, ptrdiff_t j, ptrdiff_t k) {
    if (sortCompare(c, sortAt(c, i), sortAt(c, j)) < 0) {       /* a[i] < a[j] */
        if (sortCompare(c, sortAt(c, j), sortAt(c, k)) < 0) {
            return j;                                           /* i < j < k */
        }
        return (sortCompare(c, sortAt(c, i), sortAt(c, k)) < 0) ? k : i;
    }
    /* a[j] <= a[i] */
    if (sortCompare(c, sortAt(c, i), sortAt(c, k)) < 0) {
        return i;                                               /* j <= i < k */
    }
    return (sortCompare(c, sortAt(c, j), sortAt(c, k)) < 0) ? k : j;
}

/* 고른 피벗을 a[lo]로 옮겨 둔다. 두 파티션 모두 a[lo]를 피벗으로 쓴다. */
static void choosePivot(QuickRun *q, ptrdiff_t lo, ptrdiff_t hi) {
    ptrdiff_t p = lo;

    switch (q->pivot) {
        case QUICK_PIVOT_FIRST:
            return;
        case QUICK_PIVOT_RANDOM:
            p = lo + (ptrdiff_t)minstdBelow(&q->rng, (size_t)(hi - lo + 1));
            break;
        case QUICK_PIVOT_MEDIAN3:
            if (hi - lo >= 2) { /* 원소 둘이면 고를 셋이 없다 */
                p = medianOfThree(q->c, lo, lo + (hi - lo) / 2, hi);
            }
            break;
    }
    sortSwap(q->c, lo, p);
}

/* --- 파티션 ------------------------------------------------------------- */

/* 수업 코드의 파티션. a[lo]를 피벗으로 삼아 작은 것은 왼쪽, 나머지는 오른쪽으로
 * 보내고, 피벗을 그 경계에 놓는다. 피벗의 최종 위치를 돌려준다.
 * 교환은 lo+1 이후만 건드리므로 반복하는 동안 피벗은 a[lo]에 그대로 있다. */
static ptrdiff_t partitionTwoWay(SortContext *c, ptrdiff_t lo, ptrdiff_t hi) {
    ptrdiff_t i = lo; /* a[lo+1..i]: 피벗보다 작은 구간 */
    for (ptrdiff_t j = lo + 1; j <= hi; j++) {
        if (sortCompare(c, sortAt(c, j), sortAt(c, lo)) < 0) {
            i++;
            sortSwap(c, i, j);
        }
    }
    sortSwap(c, lo, i);
    return i;
}

/* 3-way 파티션 (다익스트라의 네덜란드 국기 문제). 끝나면
 *   a[lo..lt-1] < v,   a[lt..gt] == v,   a[gt+1..hi] > v
 * 가 된다. 교환하는 동안 a[lo]가 자리를 옮기므로 피벗 값 v는 따로 복사해 든다. */
static void partitionThreeWay(QuickRun *q, ptrdiff_t lo, ptrdiff_t hi,
                              ptrdiff_t *ltOut, ptrdiff_t *gtOut) {
    SortContext *c = q->c;
    sortCopy(c, q->pivotCopy, sortAt(c, lo));

    ptrdiff_t lt = lo;     /* a[lo..lt-1]: v보다 작은 것 */
    ptrdiff_t i = lo + 1;  /* a[lt..i-1]: v와 같은 것. a[i..gt]: 아직 안 본 것 */
    ptrdiff_t gt = hi;     /* a[gt+1..hi]: v보다 큰 것 */
    while (i <= gt) {
        int r = sortCompare(c, sortAt(c, i), q->pivotCopy);
        if (r < 0) {
            sortSwap(c, lt, i);
            lt++;
            i++;
        } else if (r > 0) {
            sortSwap(c, i, gt); /* 뒤에서 가져온 원소는 아직 안 봤으므로 i는 그대로 */
            gt--;
        } else {
            i++;
        }
    }
    *ltOut = lt;
    *gtOut = gt;
}

/* --- 재귀 --------------------------------------------------------------- */

/* a[lo..hi]를 정렬한다. 수업 코드처럼 hi가 lo보다 작게 들어와도 된다. */
static void quickSortRange(QuickRun *q, ptrdiff_t lo, ptrdiff_t hi) {
    if (lo >= hi) {
        return; /* 원소 하나 이하면 이미 정렬되어 있다 */
    }
    sortEnter(q->c);
    choosePivot(q, lo, hi);
    if (q->partition == QUICK_PARTITION_TWO_WAY) {
        ptrdiff_t p = partitionTwoWay(q->c, lo, hi);
        quickSortRange(q, lo, p - 1); /* 피벗 왼쪽 */
        quickSortRange(q, p + 1, hi); /* 피벗 오른쪽 */
    } else {
        ptrdiff_t lt;
        ptrdiff_t gt;
        partitionThreeWay(q, lo, hi, &lt, &gt);
        quickSortRange(q, lo, lt - 1); /* 피벗보다 작은 쪽 */
        quickSortRange(q, gt + 1, hi); /* 피벗보다 큰 쪽. 같은 값은 끝났다 */
    }
    sortLeave(q->c);
}

void quickSortWith(void *base, size_t n, size_t size, SortCompare cmp,
                   SortStats *stats, QuickPivot pivot, QuickPartition partition) {
    SortContext c;
    if (!sortOpen(&c, base, n, size, cmp, stats)) {
        return;
    }
    QuickRun q;
    q.c = &c;
    q.pivot = pivot;
    q.partition = partition;
    minstdSeed(&q.rng, PIVOT_SEED);
    q.pivotCopy = NULL;

    c.tmp = sortAlloc(&c, size); /* 교환용 한 칸 */
    if (partition == QUICK_PARTITION_THREE_WAY) {
        q.pivotCopy = sortAlloc(&c, size); /* 피벗 값용 한 칸 */
    }
    if (c.tmp != NULL && (partition == QUICK_PARTITION_TWO_WAY || q.pivotCopy != NULL)) {
        quickSortRange(&q, 0, (ptrdiff_t)n - 1);
    }
    sortRelease(&c, q.pivotCopy, size);
    sortClose(&c);
}

void quickSort(void *base, size_t n, size_t size, SortCompare cmp, SortStats *stats) {
    quickSortWith(base, n, size, cmp, stats, QUICK_PIVOT_RANDOM, QUICK_PARTITION_TWO_WAY);
}

/* --- 실험 3의 여섯 변형 --------------------------------------------------- */

/* 구현 표에 넣으려면 SortFunction 모양이어야 해서, 조합마다 한 줄짜리 함수를 둔다. */
static void quickFirstTwoWay(void *b, size_t n, size_t s, SortCompare f, SortStats *st) {
    quickSortWith(b, n, s, f, st, QUICK_PIVOT_FIRST, QUICK_PARTITION_TWO_WAY);
}
static void quickRandomTwoWay(void *b, size_t n, size_t s, SortCompare f, SortStats *st) {
    quickSortWith(b, n, s, f, st, QUICK_PIVOT_RANDOM, QUICK_PARTITION_TWO_WAY);
}
static void quickMedianTwoWay(void *b, size_t n, size_t s, SortCompare f, SortStats *st) {
    quickSortWith(b, n, s, f, st, QUICK_PIVOT_MEDIAN3, QUICK_PARTITION_TWO_WAY);
}
static void quickFirstThreeWay(void *b, size_t n, size_t s, SortCompare f, SortStats *st) {
    quickSortWith(b, n, s, f, st, QUICK_PIVOT_FIRST, QUICK_PARTITION_THREE_WAY);
}
static void quickRandomThreeWay(void *b, size_t n, size_t s, SortCompare f, SortStats *st) {
    quickSortWith(b, n, s, f, st, QUICK_PIVOT_RANDOM, QUICK_PARTITION_THREE_WAY);
}
static void quickMedianThreeWay(void *b, size_t n, size_t s, SortCompare f, SortStats *st) {
    quickSortWith(b, n, s, f, st, QUICK_PIVOT_MEDIAN3, QUICK_PARTITION_THREE_WAY);
}

const SortAlgorithm QUICK_VARIANTS[] = {
    {"first/2way",   "O(n log n)", "O(n^2)", "O(log n)", 0, quickFirstTwoWay},
    {"random/2way",  "O(n log n)", "O(n^2)", "O(log n)", 0, quickRandomTwoWay},
    {"median3/2way", "O(n log n)", "O(n^2)", "O(log n)", 0, quickMedianTwoWay},
    {"first/3way",   "O(n log n)", "O(n^2)", "O(log n)", 0, quickFirstThreeWay},
    {"random/3way",  "O(n log n)", "O(n^2)", "O(log n)", 0, quickRandomThreeWay},
    {"median3/3way", "O(n log n)", "O(n^2)", "O(log n)", 0, quickMedianThreeWay},
};

const size_t QUICK_VARIANT_COUNT = sizeof(QUICK_VARIANTS) / sizeof(QUICK_VARIANTS[0]);
