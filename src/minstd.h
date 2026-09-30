/* 유사난수 생성기 minstd — 주제 04의 randomProgram.c와 같은 식이다.
 *
 *   x <- x * 16807 mod (2^31 - 1)
 *
 * 라이브러리 rand()는 C 구현마다 수열이 다르다. 이것을 쓰면 어느 기계에서
 * 돌려도 같은 입력 배열과 같은 피벗이 나온다. 그래서 비교 · 이동 횟수가
 * 기계와 무관하게 재현된다. 시간만 기계를 탄다.
 *
 * 전역 상태를 두지 않고 생성기마다 상태를 따로 든다. 입력을 만드는 쪽과
 * 피벗을 고르는 쪽이 서로의 수열을 흐트러뜨리지 않게 하려는 것이다.
 */
#ifndef MINSTD_H
#define MINSTD_H

#include <stddef.h>
#include <stdint.h>

typedef struct Minstd {
    uint32_t state; /* 1 이상 2^31 - 2 이하 */
} Minstd;

/* srand처럼 시드를 넣는다. 0이 되는 시드는 1로 바꾼다(0이면 영원히 0이다). */
void minstdSeed(Minstd *r, uint32_t seed);

/* rand처럼 다음 수를 만든다. 1 이상 2^31 - 2 이하. */
uint32_t minstdNext(Minstd *r);

/* 0 이상 bound 미만의 수. 나머지를 쓰므로 아주 약간 치우치지만, bound가
 * 2^31보다 훨씬 작은 이 과제에서는 무시할 만하다. bound는 1 이상이어야 한다. */
size_t minstdBelow(Minstd *r, size_t bound);

#endif /* MINSTD_H */
