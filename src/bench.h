/* 세 정렬을 같은 잣대로 재는 도구.
 *
 * 정렬(sort.c 쪽)과 측정(이 파일)을 나눠 둔다. 정렬은 자기가 측정당하는 줄
 * 모르고, 측정은 어떤 정렬인지 모른다. 둘을 잇는 것은 SortAlgorithm 하나다.
 */
#ifndef BENCH_H
#define BENCH_H

#include <stddef.h>
#include <stdint.h>

#include "sort.h"

/* 측정에 쓰는 원소. key로 정렬하고, order에는 입력에서의 순서를 새겨 둔다.
 * 정렬 뒤에도 같은 key끼리 order가 오름차순이면 그 정렬은 안정했다.
 * int 하나로는 3과 3을 구별할 수 없어서 원소를 두 칸으로 잡았다. */
typedef struct Record {
    int key;
    int order;
} Record;

/* key만 본다. order까지 보면 어떤 정렬이든 안정해 보인다. */
int compareRecordKeys(const void *a, const void *b);

/* 입력 모양. 같은 정렬도 입력에 따라 성능이 크게 달라진다. */
typedef enum InputShape {
    SHAPE_RANDOM,     /* 무작위 (값이 거의 겹치지 않는다) */
    SHAPE_SORTED,     /* 이미 정렬됨 */
    SHAPE_REVERSED,   /* 역순 */
    SHAPE_FEW_UNIQUE, /* 중복 많음: 값이 0~7 여덟 가지뿐 */
    SHAPE_COUNT
} InputShape;

enum { FEW_UNIQUE_VALUES = 8 };

const char *shapeLabel(InputShape shape); /* 표에 찍는 한글 이름 */
const char *shapeKey(InputShape shape);   /* CSV에 쓰는 ASCII 이름 */

/* a[0..n-1]을 shape 모양으로 채운다. 난수는 minstd라 seed가 같으면
 * 어느 기계에서든 같은 배열이 나온다. */
void fillInput(Record *a, size_t n, InputShape shape, uint32_t seed);

int isSortedByKey(const Record *a, size_t n);   /* key가 오름차순인가 */
int keepsInputOrder(const Record *a, size_t n); /* 같은 key끼리 order가 오름차순인가 */

typedef struct Measurement {
    const SortAlgorithm *algo;
    size_t n;
    double millis;   /* reps번 잰 시간의 가운데 값 */
    SortStats stats; /* 비교 · 이동 등. 입력이 같으면 매번 같다 */
    int sorted;      /* 결과가 정렬됐는가. 이것이 0이면 다른 숫자는 의미가 없다 */
    int stable;      /* 실제로 안정했는가 (구현 표의 주장이 아니라 실측) */
} Measurement;

/* input을 복사해 reps번 정렬하고 걸린 시간의 가운데 값을 남긴다.
 * 시계는 복사가 끝난 뒤에 켠다. 재는 것은 정렬뿐이다. */
Measurement measure(const SortAlgorithm *algo, const Record *input, size_t n, int reps);

#endif /* BENCH_H */
