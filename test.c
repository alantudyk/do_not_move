#include <inttypes.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

typedef bool (* is_swap_needed_t)(const void *, const void *);

typedef struct param_t {
    void *a, *tmp;
    size_t s;
    is_swap_needed_t is_unordered;
} param_t;

static size_t comparisons = 0, moves = 0;

static bool do_not_move_with_tmp(const param_t *const p,
                                 const bool is_tmp,
                                 const size_t offset,
                                 const size_t n) {
    bool lb = is_tmp, rb = is_tmp;
    size_t n1 = n / 2, n2 = n - n1;
    const size_t s = p->s;
    if (n1 > 1) lb = do_not_move_with_tmp(p, lb, offset, n1);
    if (n2 > 1) rb = do_not_move_with_tmp(p, rb, offset + n1 * s, n2);
    void *lp  = (lb ? p->tmp : p->a) + offset,
         *rp  = (rb ? p->tmp : p->a) + offset + n1 * s,
         *res = (lb ? p->a : p->tmp) + offset;
    is_swap_needed_t is_unordered = p->is_unordered;
    while (n1 > 0 && n2 > 0) {
        if (is_unordered(lp, rp))
            memcpy(res, rp, s), rp += s, --n2;
        else
            memcpy(res, lp, s), lp += s, --n1;
        res += s;
    }
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
    void *lp = tmp, *rp = a + n1 * s, *res = a;
    if (n2 > 1 && do_not_move_with_tmp(&p, false, 0, n2))
        memcpy(rp, tmp, n2 * s), moves += n2;
    p.a = a;
    if (n1 < 2 || !do_not_move_with_tmp(&p, false, 0, n1))
        memcpy(tmp, a, n1 * s), moves += n1;
    while (n1 > 0 && n2 > 0) {
        if (is_unordered(lp, rp))
            memcpy(res, rp, s), rp += s, --n2;
        else
            memcpy(res, lp, s), lp += s, --n1;
        res += s;
    }
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
