/* 정렬 비교 — 병합 · 퀵 · 힙. 실험 넷을 돌려 표나 CSV로 찍는다.
 *
 *   make run                         실험 1~4를 모두 표로 (1분 가까이 걸린다)
 *   ./src/main.out shapes            실험 하나만 표로
 *   ./src/main.out growth --csv      실험 하나를 CSV로 (tools/charts.py가 쓴다)
 *
 *   실험 1  shapes     입력 모양별         n = 100,000 · 무작위 · 정렬됨 · 역순 · 중복많음
 *   실험 2  growth     배가 실험           무작위 · n = 2^14 → 2^22, 두 배씩
 *   실험 3  quick      퀵 정렬 변형        피벗 3 × 파티션 2 · n = 2,500 → 20,000
 *   실험 4  heapbuild  힙 만들기는 O(n)?   무작위 · 정렬됨 · 역순 · n = 2^10 → 2^22
 *
 * 무엇을 잴지는 아래 "실험 조건" 한 곳에만 적는다. 부르는 쪽은 정렬 이름을
 * 적지 않고 구현 표(SORT_ALGORITHMS, QUICK_VARIANTS)를 훑을 뿐이다.
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bench.h"
#include "sort.h"

/* --- 실험 조건 --------------------------------------------------------- */

/* 입력 난수의 시드. 고정해 두어 매번 같은 입력을 쓴다 (값은 마감일). */
static const uint32_t INPUT_SEED = 20260930u;

/* 실험 1 */
enum { SHAPES_N = 100000, SHAPES_REPS = 5 };

/* 실험 2: n이 2의 거듭제곱이라 log2 n이 정수다. */
enum { GROWTH_MIN_EXP = 14, GROWTH_MAX_EXP = 22, GROWTH_REPS = 3 };

/* 실험 3: 첫 원소 피벗은 정렬된 입력에서 O(n^2)이라 n을 작게 잡는다. */
static const size_t QUICK_SIZES[] = {2500, 5000, 10000, 20000};
enum { QUICK_SIZE_COUNT = sizeof(QUICK_SIZES) / sizeof(QUICK_SIZES[0]), QUICK_REPS = 3 };

/* 실험 4 */
enum { HEAPBUILD_MIN_EXP = 10, HEAPBUILD_MAX_EXP = 22 };
static const InputShape HEAPBUILD_SHAPES[] = {SHAPE_RANDOM, SHAPE_SORTED, SHAPE_REVERSED};
enum { HEAPBUILD_SHAPE_COUNT = sizeof(HEAPBUILD_SHAPES) / sizeof(HEAPBUILD_SHAPES[0]) };

/* 배율을 계산하려고 알고리즘마다 직전 값을 들고 있는 칸 수. */
enum { MAX_ALGORITHMS = 8 };

/* --- 출력 도구 --------------------------------------------------------- */

/* 터미널에서 한글은 두 칸을 차지한다. printf의 %10s는 바이트를 세므로 한글이
 * 섞인 머리글은 줄이 어긋난다. 그래서 머리글만은 화면 폭을 직접 세어 채운다. */
static int displayWidth(const char *s) {
    const unsigned char *p = (const unsigned char *)s;
    int width = 0;
    while (*p != '\0') {
        if (*p < 0x80) {
            width += 1;
            p += 1;
        } else if ((*p & 0xE0) == 0xC0) {
            width += 1; /* ÷ 같은 라틴 기호 */
            p += 2;
        } else if ((*p & 0xF0) == 0xE0) {
            unsigned cp = ((p[0] & 0x0Fu) << 12) | ((p[1] & 0x3Fu) << 6) | (p[2] & 0x3Fu);
            int wide = (cp >= 0x1100 && cp <= 0x115F) || (cp >= 0x2E80 && cp <= 0xA4CF) ||
                       (cp >= 0xAC00 && cp <= 0xD7A3) || (cp >= 0xF900 && cp <= 0xFAFF) ||
                       (cp >= 0xFF00 && cp <= 0xFF60);
            width += wide ? 2 : 1;
            p += 3;
        } else {
            width += 2;
            p += 4;
        }
    }
    return width;
}

typedef struct Column {
    const char *label;
    int width;
    int left; /* 1이면 왼쪽 정렬, 0이면 오른쪽 정렬 */
} Column;

static void printHeader(const Column *cols, int count) {
    for (int i = 0; i < count; i++) {
        int pad = cols[i].width - displayWidth(cols[i].label);
        if (pad < 0) {
            pad = 0;
        }
        if (cols[i].left) {
            printf("%s%*s", cols[i].label, (i == count - 1) ? 0 : pad, "");
        } else {
            printf("%*s%s", pad, "", cols[i].label);
        }
    }
    printf("\n");
}

/* 1536567 -> "1,536,567". 한 printf 안에서 여러 번 부를 수 있게 버퍼를 돌려 쓴다. */
static const char *withCommas(size_t value) {
    static char pool[8][32];
    static int next = 0;
    char digits[24];
    char *out = pool[next];
    next = (next + 1) % 8;

    int len = snprintf(digits, sizeof digits, "%zu", value);
    int k = 0;
    for (int i = 0; i < len; i++) {
        if (i > 0 && (len - i) % 3 == 0) {
            out[k++] = ',';
        }
        out[k++] = digits[i];
    }
    out[k] = '\0';
    return out;
}

/* 배가 실험의 배율 now / before. 앞 값이 없으면 "-". */
static const char *ratioText(double now, double before) {
    static char pool[8][16];
    static int next = 0;
    char *out = pool[next];
    next = (next + 1) % 8;

    if (before <= 0.0) {
        snprintf(out, 16, "-");
    } else {
        snprintf(out, 16, "x%.2f", now / before);
    }
    return out;
}

static Record *allocRecords(size_t n) {
    Record *a = (Record *)malloc((n > 0 ? n : 1) * sizeof *a);
    if (a == NULL) {
        fprintf(stderr, "메모리가 부족하다 (원소 %zu개)\n", n);
        exit(1);
    }
    return a;
}

static void csvHeader(void) {
    printf("experiment,input,n,algo,millis,compares,moves,extraBytes,maxDepth,sorted,stable\n");
}

static void csvRow(const char *experiment, InputShape shape, const Measurement *m) {
    printf("%s,%s,%zu,%s,%.3f,%zu,%zu,%zu,%zu,%d,%d\n", experiment, shapeKey(shape), m->n,
           m->algo->name, m->millis, m->stats.compares, m->stats.moves,
           m->stats.extraBytes, m->stats.maxDepth, m->sorted, m->stable);
    fflush(stdout);
}

/* 구현 표가 무엇을 주장하는지 먼저 보여 준다. 아래 측정과 견줘 보라고. */
static void printDeclarations(void) {
    static const Column COLS[] = {
        {"알고리즘", 13, 1}, {"평균", 14, 1}, {"최악", 14, 1}, {"추가 공간", 12, 1}, {"안정성", 8, 1},
    };
    printf("구현 표가 주장하는 값 (아래 실험이 이것을 확인한다)\n");
    printHeader(COLS, 5);
    for (size_t k = 0; k < SORT_ALGORITHM_COUNT; k++) {
        const SortAlgorithm *a = &SORT_ALGORITHMS[k];
        printf("%-13s%-14s%-14s%-12s%s\n", a->name, a->averageTime, a->worstTime,
               a->extraSpace, a->stable ? "stable" : "unstable");
    }
    printf("\n원소는 (key, order) %zu바이트다. key로 정렬하고 order로 안정성을 본다.\n",
           sizeof(Record));
    printf("시간은 기계마다 다르다. 비교 · 이동 횟수는 어느 기계에서든 같다.\n");
}

/* --- 실험 1: 입력 모양별 ------------------------------------------------ */

static void runShapes(int csv) {
    static const Column COLS[] = {
        {"알고리즘", 11, 1}, {"시간(ms)", 11, 0}, {"비교", 14, 0}, {"이동", 14, 0},
        {"추가 메모리", 16, 0}, {"재귀깊이", 10, 0}, {"정렬", 6, 0}, {"안정", 6, 0},
    };
    Record *input = allocRecords(SHAPES_N);

    if (csv) {
        csvHeader();
    } else {
        printf("\n=== 실험 1. 입력 모양별 — n = %s, %d회 잰 시간의 가운데 값 ===\n",
               withCommas(SHAPES_N), SHAPES_REPS);
    }
    for (int s = 0; s < SHAPE_COUNT; s++) {
        InputShape shape = (InputShape)s;
        fillInput(input, SHAPES_N, shape, INPUT_SEED);
        if (!csv) {
            printf("\n[%s]\n", shapeLabel(shape));
            printHeader(COLS, 8);
        }
        for (size_t k = 0; k < SORT_ALGORITHM_COUNT; k++) {
            Measurement m = measure(&SORT_ALGORITHMS[k], input, SHAPES_N, SHAPES_REPS);
            if (csv) {
                csvRow("shapes", shape, &m);
                continue;
            }
            printf("%-11s%11.3f%14s%14s%14s B%10s%6s%6s\n", m.algo->name, m.millis,
                   withCommas(m.stats.compares), withCommas(m.stats.moves),
                   withCommas(m.stats.extraBytes), withCommas(m.stats.maxDepth),
                   m.sorted ? "yes" : "NO!", m.stable ? "yes" : "no");
            fflush(stdout);
        }
    }
    if (!csv) {
        printf("\n  추가 메모리: 입력 배열 밖에 잡은 작업 공간. 재귀 스택은 재귀깊이 열이 대신한다.\n");
        printf("  안정: 같은 key끼리 입력 순서가 남았는지 실측한 값. 값이 겹치지 않는 입력\n");
        printf("        (무작위 · 정렬됨 · 역순)에서는 불안정 정렬도 yes로 나온다.\n");
    }
    free(input);
}

/* --- 실험 2: 배가 실험 -------------------------------------------------- */

static void runGrowth(int csv) {
    static const Column COLS[] = {
        {"n", 10, 0}, {"  알고리즘", 13, 1}, {"시간(ms)", 10, 0}, {"시간 배율", 11, 0},
        {"비교", 14, 0}, {"비교 배율", 11, 0}, {"비교÷nlog2n", 13, 0}, {"ns÷nlog2n", 11, 0},
    };
    double lastMillis[MAX_ALGORITHMS] = {0};
    double lastCompares[MAX_ALGORITHMS] = {0};

    if (csv) {
        csvHeader();
    } else {
        printf("\n=== 실험 2. 배가 실험 — 무작위 입력, n을 두 배씩, %d회 잰 시간의 가운데 값 ===\n",
               GROWTH_REPS);
        printf("  배율: n이 두 배가 될 때 몇 배가 되었나. n log n이면 2를 조금 넘고, n^2이면 4다.\n");
        printf("  ÷nlog2n: n log2 n으로 나눈 값. 이 값이 평평하면 n log n대로 자란 것이다.\n\n");
        printHeader(COLS, 8);
    }
    for (int e = GROWTH_MIN_EXP; e <= GROWTH_MAX_EXP; e++) {
        size_t n = (size_t)1 << e;
        Record *input = allocRecords(n);
        fillInput(input, n, SHAPE_RANDOM, INPUT_SEED);
        double nLogN = (double)n * e; /* n = 2^e이므로 log2 n = e */

        for (size_t k = 0; k < SORT_ALGORITHM_COUNT && k < MAX_ALGORITHMS; k++) {
            Measurement m = measure(&SORT_ALGORITHMS[k], input, n, GROWTH_REPS);
            if (csv) {
                csvRow("growth", SHAPE_RANDOM, &m);
            } else {
                printf("%10s  %-11s%10.3f%11s%14s%11s%13.3f%11.3f\n",
                       k == 0 ? withCommas(n) : "", m.algo->name, m.millis,
                       ratioText(m.millis, lastMillis[k]), withCommas(m.stats.compares),
                       ratioText((double)m.stats.compares, lastCompares[k]),
                       (double)m.stats.compares / nLogN, m.millis * 1e6 / nLogN);
                fflush(stdout);
            }
            lastMillis[k] = m.millis;
            lastCompares[k] = (double)m.stats.compares;
        }
        free(input);
    }
}

/* --- 실험 3: 퀵 정렬 변형 ----------------------------------------------- */

static void runQuick(int csv) {
    char sizeLabels[QUICK_SIZE_COUNT][24];
    Column cols[QUICK_SIZE_COUNT + 4];
    size_t maxN = QUICK_SIZES[QUICK_SIZE_COUNT - 1];
    Record *input = allocRecords(maxN);

    cols[0] = (Column){"변형", 13, 1};
    for (int i = 0; i < QUICK_SIZE_COUNT; i++) {
        snprintf(sizeLabels[i], sizeof sizeLabels[i], "n=%s", withCommas(QUICK_SIZES[i]));
        cols[1 + i] = (Column){sizeLabels[i], 13, 0};
    }
    cols[QUICK_SIZE_COUNT + 1] = (Column){"배율", 7, 0};
    cols[QUICK_SIZE_COUNT + 2] = (Column){"재귀깊이", 10, 0};
    cols[QUICK_SIZE_COUNT + 3] = (Column){"시간(ms)", 10, 0};

    if (csv) {
        csvHeader();
    } else {
        printf("\n=== 실험 3. 퀵 정렬 — 피벗(first · random · median3) × 파티션(2way · 3way) ===\n");
        printf("  n 열의 숫자는 비교 횟수. 배율은 n이 두 배(1만 → 2만)가 될 때 비교가 몇 배가 되었나.\n");
        printf("  재귀깊이와 시간은 n = %s에서 잰 값이다. 배율이 4면 O(n^2)이다.\n", withCommas(maxN));
    }
    for (int s = 0; s < SHAPE_COUNT; s++) {
        InputShape shape = (InputShape)s;
        if (!csv) {
            printf("\n[%s]\n", shapeLabel(shape));
            printHeader(cols, QUICK_SIZE_COUNT + 4);
        }
        for (size_t v = 0; v < QUICK_VARIANT_COUNT; v++) {
            Measurement runs[QUICK_SIZE_COUNT];
            for (int i = 0; i < QUICK_SIZE_COUNT; i++) {
                size_t n = QUICK_SIZES[i];
                fillInput(input, n, shape, INPUT_SEED);
                runs[i] = measure(&QUICK_VARIANTS[v], input, n, QUICK_REPS);
                if (csv) {
                    csvRow("quick", shape, &runs[i]);
                }
            }
            if (csv) {
                continue;
            }
            const Measurement *big = &runs[QUICK_SIZE_COUNT - 1];
            const Measurement *half = &runs[QUICK_SIZE_COUNT - 2];
            printf("%-13s%13s%13s%13s%13s%7.2f%10s%10.3f%s\n", QUICK_VARIANTS[v].name,
                   withCommas(runs[0].stats.compares), withCommas(runs[1].stats.compares),
                   withCommas(runs[2].stats.compares), withCommas(big->stats.compares),
                   (double)big->stats.compares / (double)half->stats.compares,
                   withCommas(big->stats.maxDepth), big->millis,
                   big->sorted ? "" : "  정렬 실패!");
            fflush(stdout);
        }
    }
    free(input);
}

/* --- 실험 4: 힙 만들기 ------------------------------------------------- */

/* 모든 노드가 부모보다 작거나 같은가. */
static int isMaxHeap(const Record *a, size_t n) {
    for (size_t i = 1; i < n; i++) {
        if (a[(i - 1) / 2].key < a[i].key) {
            return 0;
        }
    }
    return 1;
}

static void runHeapBuild(int csv) {
    static const Column COLS[] = {
        {"n", 10, 0}, {"비교", 16, 0}, {"비교÷n", 11, 0}, {"이동÷n", 11, 0}, {"힙 성질", 10, 0},
    };
    size_t maxN = (size_t)1 << HEAPBUILD_MAX_EXP;
    Record *a = allocRecords(maxN);

    if (csv) {
        printf("experiment,input,n,compares,moves,heapValid\n");
    } else {
        printf("\n=== 실험 4. 힙 만들기(buildMaxHeap) — O(n)이면 비교÷n이 n과 무관하게 일정하다 ===\n");
    }
    for (int s = 0; s < HEAPBUILD_SHAPE_COUNT; s++) {
        InputShape shape = HEAPBUILD_SHAPES[s];
        if (!csv) {
            printf("\n[%s]\n", shapeLabel(shape));
            printHeader(COLS, 5);
        }
        for (int e = HEAPBUILD_MIN_EXP; e <= HEAPBUILD_MAX_EXP; e++) {
            size_t n = (size_t)1 << e;
            SortStats stats;
            fillInput(a, n, shape, INPUT_SEED);
            buildMaxHeap(a, n, sizeof a[0], compareRecordKeys, &stats);
            int valid = isMaxHeap(a, n);
            if (csv) {
                printf("heapbuild,%s,%zu,%zu,%zu,%d\n", shapeKey(shape), n, stats.compares,
                       stats.moves, valid);
            } else {
                printf("%10s%16s%11.3f%11.3f%10s\n", withCommas(n), withCommas(stats.compares),
                       (double)stats.compares / (double)n, (double)stats.moves / (double)n,
                       valid ? "ok" : "BROKEN");
            }
        }
    }
    if (!csv) {
        printf("\n  원소를 하나씩 넣으며 힙을 키우면 O(n log n)이라 비교÷n이 n을 따라 자란다.\n");
        printf("  아래에서 위로 내려보내면(Floyd) 비교÷n은 2를 넘지 않는다.\n");
    }
    free(a);
}

/* --- 진입점 ------------------------------------------------------------ */

typedef struct Experiment {
    const char *name;
    void (*run)(int csv);
} Experiment;

static const Experiment EXPERIMENTS[] = {
    {"shapes", runShapes},
    {"growth", runGrowth},
    {"quick", runQuick},
    {"heapbuild", runHeapBuild},
};

enum { EXPERIMENT_COUNT = sizeof(EXPERIMENTS) / sizeof(EXPERIMENTS[0]) };

static void usage(void) {
    fprintf(stderr, "사용법: main.out [all|shapes|growth|quick|heapbuild] [--csv]\n");
    fprintf(stderr, "  --csv는 실험 하나를 고를 때만 쓴다.\n");
}

int main(int argc, char **argv) {
    const char *which = "all";
    int csv = 0;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--csv") == 0) {
            csv = 1;
        } else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            usage();
            return 0;
        } else {
            which = argv[i];
        }
    }

    if (strcmp(which, "all") == 0) {
        if (csv) {
            usage();
            return 2;
        }
        printf("=== 정렬 비교: 병합 · 퀵 · 힙 ===\n\n");
        printDeclarations();
        for (int e = 0; e < EXPERIMENT_COUNT; e++) {
            EXPERIMENTS[e].run(0);
        }
        return 0;
    }
    for (int e = 0; e < EXPERIMENT_COUNT; e++) {
        if (strcmp(which, EXPERIMENTS[e].name) == 0) {
            if (!csv) {
                printDeclarations();
            }
            EXPERIMENTS[e].run(csv);
            return 0;
        }
    }
    usage();
    return 2;
}
