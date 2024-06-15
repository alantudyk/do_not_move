#include <inttypes.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

#define unlikely(x) __builtin_expect(!!(x), 0)
#define loop for (;;)

typedef bool (*const is_swap_needed_t)(const void *, const void *);

typedef struct param_t {
    void *a, *const tmp;
    const size_t s;
    is_swap_needed_t is_unordered;
} param_t;

static size_t comparisons = 0, moves = 0;

static bool do_not_move_with_tmp(const param_t *const p,
                                 const size_t offset,
                                 const size_t n) {
    size_t n1 = n / 2, n2 = n - n1;
    const size_t s = p->s, r_offset = offset + n1 * s;
    const bool lb = n1 > 1 && do_not_move_with_tmp(p, offset, n1),
               rb = n2 > 1 && do_not_move_with_tmp(p, r_offset, n2);
    const void *lp = (lb ? p->tmp : p->a) + offset,
               *rp = (rb ? p->tmp : p->a) + r_offset;
    void *res = (lb ? p->a : p->tmp) + offset;
    is_swap_needed_t is_unordered = p->is_unordered;
#define M_E_R_G_E(n1, n2) \
    loop \
        if (is_unordered(lp, rp)) { \
            memcpy(res, rp, s), rp += s; res += s; \
            if (--n2 == 0) break; \
        } else { \
            memcpy(res, lp, s), lp += s, res += s; \
            if (--n1 == 0) break; \
        }
    M_E_R_G_E(n1, n2)
    moves += n - n2;
    if (n1 > 0)
        memcpy(res, lp, n1 * s);
    else if (lb == rb)
        memcpy(res, rp, n2 * s), moves += n2;
    return !lb;
}

static void memswap(void *a, void *b, const size_t s) {
    const size_t r = s % sizeof(size_t);
    const void *const A = a + (s - r);
    size_t tmp;
    while (a < A)
        tmp = *(size_t *)a,
        *(size_t *)a = *(size_t *)b,
        *(size_t *)b = tmp,
        a += s,
        b += s;
    memcpy(&tmp, a, r),
    memcpy(a, b, r),
    memcpy(b, &tmp, r);
}

static void do_not_move(void *const a,
                        const size_t n,
                        const size_t s,
                        is_swap_needed_t is_unordered) {
    if (unlikely(n < 4)) {
        if (n < 2) return;
        if (is_unordered(a, a + s))
            memswap(a, a + s, s), moves += 2;
        if (n == 3 && is_unordered(a + s, a + s * 2)) {
            memswap(a + s, a + s * 2, s), moves += 2;
            if (is_unordered(a, a + s))
                memswap(a, a + s, s), moves += 2;
        }
        return;
    }
    size_t n12 = n / 2, n34 = n - n12;
    void *const tmp = malloc(n34 * s);
    if (unlikely(tmp == NULL)) {
        printf(
            "\n"
            "\tIt's time to die.\n"
            "\tOr time to look at https://github.com/BonzaiThePenguin/WikiSort\n"
            "\n"
        );
        exit(1);
    }
    param_t p = {
        .a = a + n12 * s,
        .tmp = tmp,
        .s = s,
        .is_unordered = is_unordered
    };
    const void *lp = tmp, *rp = a + n12 * s;
    void *res = a;
    if (n34 > 1 && do_not_move_with_tmp(&p, 0, n34))
        memcpy((void *)rp, tmp, n34 * s), moves += n34;
    p.a = a;
    if (n12 < 2 || !do_not_move_with_tmp(&p, 0, n12))
        memcpy(tmp, a, n12 * s), moves += n12;
    M_E_R_G_E(n12, n34)
    moves += n - n34;
    if (n12 > 0) memcpy(res, lp, n12 * s);
    free(tmp);
}

#define MSORT_CMP(NAME) \
bool NAME(const void *const lvp, const void *const rvp)

#define _(CND) \
    if (unlikely(CND)) { \
        fprintf(stderr, "\n\t🤔, line: %d\n\n", __LINE__); \
        exit(1); \
    }

static int cmp(const void *_a, const void *_b) {
    const int32_t *a = _a, *b = _b;
    return (*a > *b) - (*a < *b);
}

static MSORT_CMP(is_unordered) {
    ++comparisons;
    return *(const int32_t *)lvp > *(const int32_t *)rvp;
}

#define N (size_t)1e6
static int32_t R[N], A[N];

int main(void) {

    // srand(time(NULL));

    for (size_t i = 0; i < N; i++) R[i] = A[i] = rand();

    do_not_move(A, N, sizeof(int32_t), is_unordered);

    qsort(R, N, sizeof(int32_t), cmp);
    _(!!memcmp(R, A, N * sizeof(int32_t)))

    printf(
        "\n"
        "\telements: %23zu\n"
        "\tcomparisons: %20zu\n"
        "\tmoves: %26zu\n"
        "\n"
    , N, comparisons, moves);

    return 0;
}
