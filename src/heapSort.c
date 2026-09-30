/* 힙 정렬 — 이 과제의 "배우지 않은 정렬". 절차는 heapSort.pseudo에 있다.
 *
 * 최대 힙을 만든 뒤, 루트(최댓값)를 끝으로 보내고 줄어든 힙을 고치기를
 * 되풀이한다.
 *
 * 뼈대는 선택 정렬(주제 02)과 같다: "남은 것 중 가장 큰 것을 골라 뒤로 보낸다".
 * 다른 것은 고르는 방법 하나다. 선택 정렬은 남은 것을 다 훑어 O(n)이 들고,
 * 힙 정렬은 루트를 집은 뒤 한 줄기만 고쳐 O(log n)이 든다. 그래서 전체가
 * O(n^2)에서 O(n log n)이 된다.
 *
 * 배열을 그대로 완전 이진 트리로 읽는다. 포인터가 없다.
 *   a[i]의 자식: a[2i+1], a[2i+2]      a[i]의 부모: a[(i-1)/2]
 *
 *   - 추가 공간은 교환이 거쳐 가는 원소 한 칸뿐이고, 재귀도 없다: O(1)
 *   - 루트와 끝을 맞바꿀 때 사이의 같은 값을 뛰어넘는다: 불안정
 *   - 자식이 2i+1에 있어 트리 아래로 갈수록 멀리 뛴다: 배열이 캐시보다 커지면
 *     느려진다 (실험 2의 "시간 ÷ n log n")
 */
#include "sort.h"

#include "sortutil.h"

/* a[i]를 더 큰 자식과 맞바꾸며 내려보낸다. a[0..n-1]만 힙으로 본다.
 * 한 층에 비교 2회(두 자식과), 교환 1회(이동 3회). 반복문이라 재귀가 없다. */
static void siftDown(SortContext *c, ptrdiff_t i, ptrdiff_t n) {
    for (;;) {
        ptrdiff_t largest = i;
        ptrdiff_t left = 2 * i + 1;
        ptrdiff_t right = left + 1;

        if (left < n && sortCompare(c, sortAt(c, left), sortAt(c, largest)) > 0) {
            largest = left;
        }
        if (right < n && sortCompare(c, sortAt(c, right), sortAt(c, largest)) > 0) {
            largest = right;
        }
        if (largest == i) {
            return; /* 두 자식보다 크거나 같다: 힙 성질이 섰다 */
        }
        sortSwap(c, i, largest);
        i = largest;
    }
}

/* 1단계: 잎이 아닌 마지막 노드부터 거꾸로 올라가며 내려보낸다(Floyd).
 * 원소를 하나씩 넣으며 올려보내면 O(n log n)이지만, 이렇게 하면 O(n)이다.
 * 대부분의 노드가 잎 근처에 있어 조금만 내려가면 되기 때문이다. */
static void buildHeap(SortContext *c, ptrdiff_t n) {
    for (ptrdiff_t i = n / 2 - 1; i >= 0; i--) {
        siftDown(c, i, n);
    }
}

void heapSort(void *base, size_t n, size_t size, SortCompare cmp, SortStats *stats) {
    SortContext c;
    if (!sortOpen(&c, base, n, size, cmp, stats)) {
        return;
    }
    c.tmp = sortAlloc(&c, size); /* 추가 공간은 이 한 칸뿐이다 */
    if (c.tmp != NULL) {
        ptrdiff_t count = (ptrdiff_t)n;
        buildHeap(&c, count);
        /* 2단계: 최댓값(루트)을 끝으로 보내고, 하나 줄어든 힙의 루트를 고친다. */
        for (ptrdiff_t end = count - 1; end > 0; end--) {
            sortSwap(&c, 0, end);
            siftDown(&c, 0, end);
        }
    }
    sortClose(&c);
}

void buildMaxHeap(void *base, size_t n, size_t size, SortCompare cmp, SortStats *stats) {
    SortContext c;
    if (!sortOpen(&c, base, n, size, cmp, stats)) {
        return;
    }
    c.tmp = sortAlloc(&c, size);
    if (c.tmp != NULL) {
        buildHeap(&c, (ptrdiff_t)n);
    }
    sortClose(&c);
}
