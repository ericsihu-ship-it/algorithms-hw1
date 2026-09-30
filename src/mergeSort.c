/* 병합 정렬 — 반으로 나눠 각각 정렬하고, 정렬된 두 절반을 합친다 (주제 03).
 *
 * 수업 코드(algorithm-code의 mergeSort.c)를 원소 타입과 무관하게 옮겼다.
 * 절차는 mergeSort.pseudo 그대로다.
 *
 *   - 병합마다 구간 전체가 temp로 갔다가 돌아온다. 그래서 이동 횟수는 입력
 *     모양과 무관하게 n = 10에서 늘 68회다.
 *   - temp는 처음에 n칸을 한 번 잡는다. 추가 공간 O(n)이 이 배열이다.
 *   - 비교가 <=라서 같은 값이면 왼쪽 것이 먼저 나온다. 그래서 안정 정렬이다.
 */
#include "sort.h"

#include "sortutil.h"

/* temp[k]의 주소. temp는 a와 같은 크기의 칸으로 나뉜다. */
static char *tempAt(const SortContext *c, char *temp, ptrdiff_t k) {
    return temp + k * (ptrdiff_t)c->size;
}

/* a[lo..mid]와 a[mid+1..hi]가 각각 정렬된 상태에서 하나로 합친다. */
static void merge(SortContext *c, char *temp, ptrdiff_t lo, ptrdiff_t mid, ptrdiff_t hi) {
    ptrdiff_t i = lo;
    ptrdiff_t j = mid + 1;
    ptrdiff_t k = lo;

    while (i <= mid && j <= hi) {
        /* '<='라서 같은 값이면 왼쪽이 먼저: 안정. '<'로 바꾸면 테스트가 잡는다. */
        if (sortCompare(c, sortAt(c, i), sortAt(c, j)) <= 0) {
            sortCopy(c, tempAt(c, temp, k), sortAt(c, i));
            i++;
        } else {
            sortCopy(c, tempAt(c, temp, k), sortAt(c, j));
            j++;
        }
        k++;
    }
    /* 한쪽이 바닥나면 남은 쪽은 비교 없이 부어 넣는다. */
    while (i <= mid) {
        sortCopy(c, tempAt(c, temp, k), sortAt(c, i));
        i++;
        k++;
    }
    while (j <= hi) {
        sortCopy(c, tempAt(c, temp, k), sortAt(c, j));
        j++;
        k++;
    }
    for (k = lo; k <= hi; k++) {
        sortCopy(c, sortAt(c, k), tempAt(c, temp, k));
    }
}

/* a[lo..hi]를 정렬한다. */
static void mergeSortRange(SortContext *c, char *temp, ptrdiff_t lo, ptrdiff_t hi) {
    if (lo >= hi) {
        return; /* 원소 하나면 이미 정렬되어 있다 */
    }
    sortEnter(c);
    ptrdiff_t mid = lo + (hi - lo) / 2;
    mergeSortRange(c, temp, lo, mid);
    mergeSortRange(c, temp, mid + 1, hi);
    merge(c, temp, lo, mid, hi);
    sortLeave(c);
}

void mergeSort(void *base, size_t n, size_t size, SortCompare cmp, SortStats *stats) {
    SortContext c;
    if (!sortOpen(&c, base, n, size, cmp, stats)) {
        return;
    }
    char *temp = sortAlloc(&c, n * size); /* 추가 공간 O(n): 이 한 덩어리가 전부다 */
    if (temp != NULL) {
        mergeSortRange(&c, temp, 0, (ptrdiff_t)n - 1);
        sortRelease(&c, temp, n * size);
    }
    sortClose(&c);
}
