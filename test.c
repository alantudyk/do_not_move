#include <inttypes.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define loop for (;;)

typedef bool (*const is_swap_needed_t)(const void *, const void *);

typedef struct param_t {
    void *a, *const tmp;
    const size_t s;
    is_swap_needed_t is_unordered;
} param_t;

static size_t comparisons = 0, moves = 0;

static bool do_not_move_with_tmp(const param_t *const p,
                                 const bool is_tmp,
                                 const size_t offset,
                                 const size_t n) {
    size_t n1 = n / 2, n2 = n - n1;
    const size_t s = p->s, r_offset = offset + n1 * s;
    const bool lb = n1 > 1 ? do_not_move_with_tmp(p, is_tmp, offset, n1) : is_tmp,
               rb = n2 > 1 ? do_not_move_with_tmp(p, is_tmp, r_offset, n2) : is_tmp;
    const void *lp = (lb ? p->tmp : p->a) + offset,
               *rp = (rb ? p->tmp : p->a) + r_offset;
    void *res = (lb ? p->a : p->tmp) + offset;
    is_swap_needed_t is_unordered = p->is_unordered;
#define M_E_R_G_E \
    loop \
        if (is_unordered(lp, rp)) { \
            memcpy(res, rp, s), rp += s; res += s; \
            if (--n2 == 0) break; \
        } else { \
            memcpy(res, lp, s), lp += s, res += s; \
            if (--n1 == 0) break; \
        }
    M_E_R_G_E
    moves += n - n2;
    if (n1 > 0)
        memcpy(res, lp, n1 * s);
    else if (lb == rb)
        memcpy(res, rp, n2 * s), moves += n2;
    return !lb;
}

static void do_not_move(void *const a,
                        const size_t n,
                        const size_t s,
                        is_swap_needed_t is_unordered) {
    if (n < 2) return;
    size_t n1 = n / 2, n2 = n - n1;
    void *const tmp = malloc(n2 * s);
    if (__builtin_expect(tmp == NULL, 0)) {
        printf(
            "\n"
            "\tIt's time to die.\n"
            "\tOr time to look at https://github.com/BonzaiThePenguin/WikiSort\n"
            "\n"
        );
        exit(1);
    }
    param_t p = {
        .a = a + n1 * s,
        .tmp = tmp,
        .s = s,
        .is_unordered = is_unordered
    };
    const void *lp = tmp, *rp = a + n1 * s;
    void *res = a;
    if (n2 > 1 && do_not_move_with_tmp(&p, false, 0, n2))
        memcpy((void *)rp, tmp, n2 * s), moves += n2;
    p.a = a;
    if (n1 < 2 || !do_not_move_with_tmp(&p, false, 0, n1))
        memcpy(tmp, a, n1 * s), moves += n1;
    M_E_R_G_E
    moves += n - n2;
    if (n1 > 0) memcpy(res, lp, n1 * s);
    free(tmp);
}

#define MSORT_CMP(NAME) \
bool NAME(const void *const lvp, const void *const rvp)

#define _(CND) \
    if (__builtin_expect(!!(CND), 0)) { \
        fprintf(stderr, "\n\t🤔, line: %d\n\n", __LINE__); \
        return 1; \
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
