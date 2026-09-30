/* 유닛 테스트 — 외부 프레임워크 없이 표준 C만 쓴다. 실행: make test
 *
 * 구현 표 두 개(SORT_ALGORITHMS, QUICK_VARIANTS)를 훑으며 모든 정렬에 같은
 * 검사를 돌린다. 정렬을 하나 더 넣어도 이 파일은 고칠 것이 없다.
 *
 *   1. 기본 케이스     빈 배열 · 원소 하나 · 수업 예제 · 정렬됨 · 역순 · 중복 · 같은 값 · 극단값
 *   2. 크기 훑기       n = 0..300 전부를 qsort 결과와 맞춰 본다 (블록 · 경계 실수를 잡는다)
 *   3. 입력 모양       네 입력 모양 × n = 5,000
 *   4. 안정성 주장     구현 표의 stable 값이 실측과 같은가
 *   5. 수업 숫자       슬라이드의 비교 · 이동 횟수가 그대로 나오는가 (세는 규칙 검증)
 *   6. 측정값          추가 메모리 · 재귀 깊이가 설계대로인가
 *   7. 힙 만들기       힙 성질, 그리고 비교가 2n을 넘지 않는가
 *   8. 도구            minstd 수열 · 입력 모양 · 안정성 판정기
 */
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bench.h"
#include "minstd.h"
#include "sort.h"

static int checks = 0;
static int failures = 0;

static void check(const char *who, const char *what, int ok) {
    checks++;
    if (ok) {
        printf("ok    %-14s %s\n", who, what);
        return;
    }
    failures++;
    printf("FAIL  %-14s %s\n", who, what);
}

/* 모든 정렬(본 비교 셋 + 퀵 변형 여섯)에 fn을 돌린다. */
static void forEachAlgorithm(void (*fn)(const SortAlgorithm *algo)) {
    for (size_t k = 0; k < SORT_ALGORITHM_COUNT; k++) {
        fn(&SORT_ALGORITHMS[k]);
    }
    for (size_t k = 0; k < QUICK_VARIANT_COUNT; k++) {
        fn(&QUICK_VARIANTS[k]);
    }
}

static void printInts(const char *label, const int *a, size_t n) {
    printf("      %s:", label);
    for (size_t i = 0; i < n; i++) {
        printf(" %d", a[i]);
    }
    printf("\n");
}

/* --- 1. 기본 케이스 ---------------------------------------------------- */

typedef struct IntCase {
    const char *name;
    int values[12];
    size_t n;
} IntCase;

static const IntCase INT_CASES[] = {
    {"빈 배열", {0}, 0},
    {"원소 하나", {42}, 1},
    {"원소 둘 (역순)", {2, 1}, 2},
    {"수업 예제 배열", {2, 8, 5, 9, 1, 10, 7, 6, 4, 3}, 10},
    {"이미 정렬됨", {1, 2, 3, 4, 5, 6, 7, 8, 9, 10}, 10},
    {"역순", {10, 9, 8, 7, 6, 5, 4, 3, 2, 1}, 10},
    {"중복", {3, 1, 3, 1, 2, 2, 3, 1, 2, 3, 1}, 11},
    {"모두 같은 값", {5, 5, 5, 5, 5, 5, 5}, 7},
    {"음수와 극단값", {0, -1, INT_MAX, INT_MIN, 7, -7, 0, INT_MAX}, 8},
};

static void testBasicCases(const SortAlgorithm *algo) {
    const size_t count = sizeof(INT_CASES) / sizeof(INT_CASES[0]);
    int ok = 1;

    for (size_t c = 0; c < count; c++) {
        const IntCase *tc = &INT_CASES[c];
        int got[12];
        int want[12];
        SortStats stats;

        memcpy(got, tc->values, sizeof got);
        memcpy(want, tc->values, sizeof want);
        qsort(want, tc->n, sizeof want[0], sortCompareInt);
        algo->sort(got, tc->n, sizeof got[0], sortCompareInt, &stats);

        if (tc->n > 0 && memcmp(got, want, tc->n * sizeof got[0]) != 0) {
            ok = 0;
            printf("      [%s]\n", tc->name);
            printInts("got ", got, tc->n);
            printInts("want", want, tc->n);
        }
    }
    check(algo->name, "기본 케이스 9가지를 qsort와 같게 정렬한다", ok);
}

/* stats에 NULL을 넘겨도 정렬은 한다. */
static void testNullStats(const SortAlgorithm *algo) {
    int a[] = {4, 1, 3, 1, 2};
    const int want[] = {1, 1, 2, 3, 4};
    algo->sort(a, 5, sizeof a[0], sortCompareInt, NULL);
    check(algo->name, "stats가 NULL이어도 정렬한다", memcmp(a, want, sizeof a) == 0);
}

/* --- 2. 크기 훑기 ------------------------------------------------------ */

/* 정렬 결과의 key 순서가 qsort 결과와 같은가. qsort는 안정하지 않아서
 * order까지 견줄 수는 없다. 안정성은 4번에서 따로 본다. */
static int sameKeysAsQsort(const SortAlgorithm *algo, Record *input, size_t n) {
    Record *got = malloc((n > 0 ? n : 1) * sizeof *got);
    Record *want = malloc((n > 0 ? n : 1) * sizeof *want);
    SortStats stats;
    int ok = 1;

    if (n > 0) {
        memcpy(got, input, n * sizeof *got);
        memcpy(want, input, n * sizeof *want);
    }
    qsort(want, n, sizeof *want, compareRecordKeys);
    algo->sort(got, n, sizeof *got, compareRecordKeys, &stats);
    for (size_t i = 0; i < n; i++) {
        if (got[i].key != want[i].key) {
            ok = 0;
            break;
        }
    }
    free(got);
    free(want);
    return ok;
}

static void testManySizes(const SortAlgorithm *algo) {
    enum { MAX_N = 300 };
    Record a[MAX_N];
    Minstd rng;
    int ok = 1;

    minstdSeed(&rng, 7);
    for (size_t n = 0; n <= MAX_N && ok; n++) {
        for (size_t i = 0; i < n; i++) {
            a[i].key = (int)minstdBelow(&rng, 20); /* 중복이 많은 좁은 범위 */
            a[i].order = (int)i;
        }
        if (!sameKeysAsQsort(algo, a, n)) {
            ok = 0;
            printf("      n = %zu에서 어긋났다\n", n);
        }
    }
    check(algo->name, "n = 0..300 전부 qsort와 같은 순서", ok);
}

/* --- 3. 입력 모양 ------------------------------------------------------ */

static void testShapes(const SortAlgorithm *algo) {
    enum { N = 5000 };
    Record *input = malloc(N * sizeof *input);
    int ok = 1;

    for (int s = 0; s < SHAPE_COUNT; s++) {
        fillInput(input, N, (InputShape)s, 20260930u);
        if (!sameKeysAsQsort(algo, input, N)) {
            ok = 0;
            printf("      [%s]에서 어긋났다\n", shapeLabel((InputShape)s));
        }
    }
    free(input);
    check(algo->name, "네 입력 모양 × n = 5,000", ok);
}

/* --- 4. 안정성 주장 ---------------------------------------------------- */

/* 중복이 많은 입력 20개로 잰다. 전부에서 순서를 지키면 안정, 하나라도 깨면 불안정.
 * 불안정한 정렬도 운 좋게 한 입력에서는 순서를 지킬 수 있어서 여러 번 본다. */
static int measuredStable(const SortAlgorithm *algo) {
    enum { TRIALS = 20, MAX_N = 400 };
    Record a[MAX_N];
    Minstd rng;
    SortStats stats;

    minstdSeed(&rng, 11);
    for (int t = 0; t < TRIALS; t++) {
        size_t n = 20 + (size_t)t * 19;
        for (size_t i = 0; i < n; i++) {
            a[i].key = (int)minstdBelow(&rng, 5); /* key가 0~4뿐: 같은 key가 많다 */
            a[i].order = (int)i;
        }
        algo->sort(a, n, sizeof a[0], compareRecordKeys, &stats);
        if (!isSortedByKey(a, n) || !keepsInputOrder(a, n)) {
            return 0;
        }
    }
    return 1;
}

static void testStabilityClaim(const SortAlgorithm *algo) {
    int measured = measuredStable(algo);
    char what[96];
    snprintf(what, sizeof what, "안정성: 표의 주장(%s)과 실측(%s)이 같다",
             algo->stable ? "안정" : "불안정", measured ? "안정" : "불안정");
    check(algo->name, what, measured == algo->stable);
}

/* --- 5. 수업 숫자 ------------------------------------------------------ */

/* 슬라이드의 표와 같은 비교 · 이동 횟수가 나오면, 원소 타입과 무관하게 옮긴
 * 이 구현이 수업 코드와 같은 절차를 같은 규칙으로 센다는 뜻이다. */
typedef struct LectureCase {
    const char *who;
    const char *input;
    int values[10];
    size_t compares;
    size_t moves;
} LectureCase;

static const LectureCase LECTURE_CASES[] = {
    /* 주제 03 "입력을 바꾸면" */
    {"mergeSort", "예제 배열", {2, 8, 5, 9, 1, 10, 7, 6, 4, 3}, 22, 68},
    {"mergeSort", "이미 정렬됨", {1, 2, 3, 4, 5, 6, 7, 8, 9, 10}, 19, 68},
    {"mergeSort", "역순", {10, 9, 8, 7, 6, 5, 4, 3, 2, 1}, 15, 68},
    /* 주제 04 "입력을 바꾸면" (첫 원소 피벗 · 2-way 파티션 = 수업 코드) */
    {"first/2way", "최선을 만드는 배열", {5, 1, 3, 4, 2, 8, 7, 6, 9, 10}, 19, 9},
    {"first/2way", "예제 배열", {2, 8, 5, 9, 1, 10, 7, 6, 4, 3}, 25, 21},
    {"first/2way", "이미 정렬됨", {1, 2, 3, 4, 5, 6, 7, 8, 9, 10}, 45, 0},
    {"first/2way", "역순", {10, 9, 8, 7, 6, 5, 4, 3, 2, 1}, 45, 15},
};

static const SortAlgorithm *findAlgorithm(const char *name) {
    for (size_t k = 0; k < SORT_ALGORITHM_COUNT; k++) {
        if (strcmp(SORT_ALGORITHMS[k].name, name) == 0) {
            return &SORT_ALGORITHMS[k];
        }
    }
    for (size_t k = 0; k < QUICK_VARIANT_COUNT; k++) {
        if (strcmp(QUICK_VARIANTS[k].name, name) == 0) {
            return &QUICK_VARIANTS[k];
        }
    }
    return NULL;
}

static void testLectureNumbers(void) {
    const size_t count = sizeof(LECTURE_CASES) / sizeof(LECTURE_CASES[0]);
    for (size_t c = 0; c < count; c++) {
        const LectureCase *lc = &LECTURE_CASES[c];
        const SortAlgorithm *algo = findAlgorithm(lc->who);
        int a[10];
        SortStats stats;
        char what[128];

        memcpy(a, lc->values, sizeof a);
        algo->sort(a, 10, sizeof a[0], sortCompareInt, &stats);
        snprintf(what, sizeof what, "수업 숫자 [%s] 비교 %zu · 이동 %zu (실제 %zu · %zu)",
                 lc->input, lc->compares, lc->moves, stats.compares, stats.moves);
        check(lc->who, what, stats.compares == lc->compares && stats.moves == lc->moves);
    }
}

/* --- 6. 측정값 --------------------------------------------------------- */

static void testStats(void) {
    int example[10] = {2, 8, 5, 9, 1, 10, 7, 6, 4, 3};
    int a[10];
    SortStats stats;

    memcpy(a, example, sizeof a);
    mergeSort(a, 10, sizeof a[0], sortCompareInt, &stats);
    check("mergeSort", "추가 메모리 = temp 배열 n칸 (40 B)", stats.extraBytes == 10 * sizeof(int));
    check("mergeSort", "재귀 깊이 = 레벨 수 4 (n = 10)", stats.maxDepth == 4);

    memcpy(a, example, sizeof a);
    heapSort(a, 10, sizeof a[0], sortCompareInt, &stats);
    check("heapSort", "추가 메모리 = 교환용 한 칸 (4 B)", stats.extraBytes == sizeof(int));
    check("heapSort", "재귀 없음: 깊이 1", stats.maxDepth == 1);

    memcpy(a, example, sizeof a);
    quickSortWith(a, 10, sizeof a[0], sortCompareInt, &stats, QUICK_PIVOT_RANDOM,
                  QUICK_PARTITION_TWO_WAY);
    check("quickSort", "2-way 추가 메모리 = 교환용 한 칸 (4 B)", stats.extraBytes == sizeof(int));

    memcpy(a, example, sizeof a);
    quickSortWith(a, 10, sizeof a[0], sortCompareInt, &stats, QUICK_PIVOT_RANDOM,
                  QUICK_PARTITION_THREE_WAY);
    check("quickSort", "3-way 추가 메모리 = 교환용 + 피벗 값 두 칸 (8 B)",
          stats.extraBytes == 2 * sizeof(int));

    int sorted[10] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    quickSortWith(sorted, 10, sizeof sorted[0], sortCompareInt, &stats, QUICK_PIVOT_FIRST,
                  QUICK_PARTITION_TWO_WAY);
    check("first/2way", "정렬된 입력에서 트리가 외길: 깊이 n-1 = 9", stats.maxDepth == 9);

    int one[1] = {7};
    mergeSort(one, 1, sizeof one[0], sortCompareInt, &stats);
    check("mergeSort", "원소 하나: 비교 0 · 이동 0 · 메모리 0",
          stats.compares == 0 && stats.moves == 0 && stats.extraBytes == 0);
}

/* 같은 입력이면 비교 · 이동 횟수가 매번 같다 (랜덤 피벗도 시드가 고정). */
static void testReproducible(const SortAlgorithm *algo) {
    enum { N = 2000 };
    Record *input = malloc(N * sizeof *input);
    Record *a = malloc(N * sizeof *a);
    SortStats first;
    SortStats second;

    fillInput(input, N, SHAPE_RANDOM, 3);
    memcpy(a, input, N * sizeof *a);
    algo->sort(a, N, sizeof *a, compareRecordKeys, &first);
    memcpy(a, input, N * sizeof *a);
    algo->sort(a, N, sizeof *a, compareRecordKeys, &second);
    check(algo->name, "같은 입력이면 비교 · 이동 횟수가 매번 같다",
          first.compares == second.compares && first.moves == second.moves);
    free(input);
    free(a);
}

/* --- 7. 힙 만들기 ------------------------------------------------------ */

static int isMaxHeap(const Record *a, size_t n) {
    for (size_t i = 1; i < n; i++) {
        if (a[(i - 1) / 2].key < a[i].key) {
            return 0;
        }
    }
    return 1;
}

static void testBuildMaxHeap(void) {
    enum { MAX_N = 1024 };
    Record a[MAX_N];
    SortStats stats;
    int heapOk = 1;
    int boundOk = 1;

    for (int s = 0; s < SHAPE_COUNT; s++) {
        for (size_t n = 0; n <= MAX_N; n += (n < 64) ? 1 : 97) {
            fillInput(a, n, (InputShape)s, 5);
            buildMaxHeap(a, n, sizeof a[0], compareRecordKeys, &stats);
            if (!isMaxHeap(a, n)) {
                heapOk = 0;
                printf("      [%s] n = %zu에서 힙 성질이 깨졌다\n", shapeLabel((InputShape)s), n);
            }
            if (stats.compares > 2 * n) {
                boundOk = 0;
                printf("      [%s] n = %zu에서 비교 %zu > 2n\n", shapeLabel((InputShape)s), n,
                       stats.compares);
            }
        }
    }
    check("buildMaxHeap", "모든 노드가 부모보다 작거나 같다 (힙 성질)", heapOk);
    check("buildMaxHeap", "비교가 2n을 넘지 않는다 (O(n))", boundOk);
}

/* --- 8. 도구 ----------------------------------------------------------- */

static void testMinstd(void) {
    Minstd r;
    minstdSeed(&r, 1);
    uint32_t a = minstdNext(&r);
    uint32_t b = minstdNext(&r);
    uint32_t c = minstdNext(&r);
    check("minstd", "시드 1: 16807 282475249 1622650073 (주제 04와 같다)",
          a == 16807u && b == 282475249u && c == 1622650073u);

    minstdSeed(&r, 2);
    a = minstdNext(&r);
    b = minstdNext(&r);
    c = minstdNext(&r);
    check("minstd", "시드 2: 33614 564950498 1097816499 (주제 04와 같다)",
          a == 33614u && b == 564950498u && c == 1097816499u);

    minstdSeed(&r, 0);
    check("minstd", "시드 0은 1로 바뀐다 (0이면 수열이 0에 갇힌다)", minstdNext(&r) == 16807u);
}

static void testInputShapes(void) {
    enum { N = 64 };
    Record a[N];
    int ok = 1;

    fillInput(a, N, SHAPE_SORTED, 1);
    for (size_t i = 1; i < N; i++) {
        ok = ok && a[i - 1].key < a[i].key;
    }
    fillInput(a, N, SHAPE_REVERSED, 1);
    for (size_t i = 1; i < N; i++) {
        ok = ok && a[i - 1].key > a[i].key;
    }
    fillInput(a, N, SHAPE_FEW_UNIQUE, 1);
    for (size_t i = 0; i < N; i++) {
        ok = ok && a[i].key >= 0 && a[i].key < FEW_UNIQUE_VALUES && a[i].order == (int)i;
    }
    check("bench", "입력 모양: 정렬됨 · 역순 · 중복많음(0~7)이 설계대로", ok);

    Record x[N];
    Record y[N];
    fillInput(x, N, SHAPE_RANDOM, 99);
    fillInput(y, N, SHAPE_RANDOM, 99);
    check("bench", "같은 시드면 같은 무작위 입력", memcmp(x, y, sizeof x) == 0);
}

static void testStabilityDetector(void) {
    Record kept[] = {{1, 0}, {1, 2}, {2, 1}};
    Record broken[] = {{1, 2}, {1, 0}, {2, 1}};
    check("bench", "안정성 판정기가 순서가 남은 것을 안정으로 본다", keepsInputOrder(kept, 3));
    check("bench", "안정성 판정기가 뒤집힌 것을 잡는다", !keepsInputOrder(broken, 3));
}

int main(void) {
    printf("--- 1 · 2 · 3. 정렬 결과\n");
    forEachAlgorithm(testBasicCases);
    forEachAlgorithm(testNullStats);
    forEachAlgorithm(testManySizes);
    forEachAlgorithm(testShapes);

    printf("\n--- 4. 안정성 주장\n");
    forEachAlgorithm(testStabilityClaim);

    printf("\n--- 5. 수업 숫자 재현\n");
    testLectureNumbers();

    printf("\n--- 6. 측정값\n");
    testStats();
    forEachAlgorithm(testReproducible);

    printf("\n--- 7. 힙 만들기\n");
    testBuildMaxHeap();

    printf("\n--- 8. 도구\n");
    testMinstd();
    testInputShapes();
    testStabilityDetector();

    printf("\n%d checks, %d failures\n", checks, failures);
    return failures == 0 ? 0 : 1;
}
