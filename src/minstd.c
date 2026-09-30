#include "minstd.h"

enum { MINSTD_MULTIPLIER = 16807 };
#define MINSTD_MODULUS 2147483647u /* 2^31 - 1, 소수 */

void minstdSeed(Minstd *r, uint32_t seed) {
    r->state = seed % MINSTD_MODULUS;
    if (r->state == 0) {
        r->state = 1;
    }
}

uint32_t minstdNext(Minstd *r) {
    /* 곱은 2^31 * 16807 < 2^46이라 64비트에 넉넉히 들어간다. */
    r->state = (uint32_t)((uint64_t)r->state * MINSTD_MULTIPLIER % MINSTD_MODULUS);
    return r->state;
}

size_t minstdBelow(Minstd *r, size_t bound) {
    return (size_t)minstdNext(r) % bound;
}
