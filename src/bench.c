#include "bench.h"

#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "minstd.h"

enum { MAX_REPS = 15 };

int compareRecordKeys(const void *a, const void *b) {
    int x = ((const Record *)a)->key;
    int y = ((const Record *)b)->key;
    return (x > y) - (x < y);
}

const char *shapeLabel(InputShape shape) {
    switch (shape) {
        case SHAPE_RANDOM:     return "무작위";
        case SHAPE_SORTED:     return "정렬됨";
        case SHAPE_REVERSED:   return "역순";
        case SHAPE_FEW_UNIQUE: return "중복많음";
        default:               return "?";
    }
}

const char *shapeKey(InputShape shape) {
    switch (shape) {
        case SHAPE_RANDOM:     return "random";
        case SHAPE_SORTED:     return "sorted";
        case SHAPE_REVERSED:   return "reversed";
        case SHAPE_FEW_UNIQUE: return "few-unique";
        default:               return "unknown";
    }
}

void fillInput(Record *a, size_t n, InputShape shape, uint32_t seed) {
    Minstd rng;
    minstdSeed(&rng, seed);
    for (size_t i = 0; i < n; i++) {
        switch (shape) {
            case SHAPE_RANDOM:     a[i].key = (int)minstdNext(&rng); break;
            case SHAPE_SORTED:     a[i].key = (int)i; break;
            case SHAPE_REVERSED:   a[i].key = (int)(n - i); break;
            case SHAPE_FEW_UNIQUE: a[i].key = (int)minstdBelow(&rng, FEW_UNIQUE_VALUES); break;
            default:               a[i].key = 0; break;
        }
        a[i].order = (int)i; /* 입력 순서를 새겨 둔다 */
    }
}

int isSortedByKey(const Record *a, size_t n) {
    for (size_t i = 1; i < n; i++) {
        if (a[i - 1].key > a[i].key) {
            return 0;
        }
    }
    return 1;
}

int keepsInputOrder(const Record *a, size_t n) {
    for (size_t i = 1; i < n; i++) {
        if (a[i - 1].key == a[i].key && a[i - 1].order > a[i].order) {
            return 0;
        }
    }
    return 1;
}

static int compareDoubles(const void *a, const void *b) {
    double x = *(const double *)a;
    double y = *(const double *)b;
    return (x > y) - (x < y);
}

Measurement measure(const SortAlgorithm *algo, const Record *input, size_t n, int reps) {
    Measurement m;
    m.algo = algo;
    m.n = n;
    m.millis = 0.0;
    memset(&m.stats, 0, sizeof m.stats);
    m.sorted = 0;
    m.stable = 0;

    if (reps < 1) {
        reps = 1;
    }
    if (reps > MAX_REPS) {
        reps = MAX_REPS;
    }
    Record *work = (Record *)malloc((n > 0 ? n : 1) * sizeof *work);
    if (work == NULL) {
        return m;
    }

    double times[MAX_REPS];
    for (int t = 0; t < reps; t++) {
        if (n > 0) {
            memcpy(work, input, n * sizeof *work);
        }
        /* 복사가 끝난 뒤에 시계를 켠다. 재는 것은 정렬뿐이다. */
        clock_t begin = clock();
        algo->sort(work, n, sizeof *work, compareRecordKeys, &m.stats);
        clock_t end = clock();
        times[t] = (double)(end - begin) * 1000.0 / CLOCKS_PER_SEC;
    }
    /* 한 번 튄 값에 흔들리지 않게 가운데 값을 쓴다. */
    qsort(times, (size_t)reps, sizeof times[0], compareDoubles);
    m.millis = times[reps / 2];

    m.sorted = isSortedByKey(work, n);
    m.stable = keepsInputOrder(work, n);
    free(work);
    return m;
}
