#include "zeno-config-deriv.h"

#if MAX_PEERS > 0

#include <assert.h>
#include "binheap.h"

/* for seq_l() */
#include "zeno-int.h"

static void minseqheap_heapify(peeridx_t j, peeridx_t n, peeridx_t * restrict p, minseqheap_idx_t * restrict q, const seq_t * restrict v)
{
    peeridx_t k;
    /* j < n/2 term to protect against overflow of k:
       - for n even: j < n/2 => k < 2*(n/2)+1 = n+1
       - for n odd:  j < n/2 = (n-1)/2 => k < 2*((n-1)/2)+1 = n 
       n+1 potentially overflows, but j < n/2 takes care of that
       --
       in the loop body k can only be incremented if k+1 < n, 
       but (1) k < n and (2) n is in range, so k+1 is
       at most n and therefore also in range */
    for (k = 2*j+1; j < n/2 && k < n; j = k, k += k + 1) {
        if (k+1 < n && seq_lt(v[p[k+1]], v[p[k]])) {
            k++;
        }
        if (seq_lt(v[p[k]], v[p[j]])) {
            peeridx_t t;
            t = p[j]; p[j] = p[k]; p[k] = t;
            q[p[j]].i = j; q[p[k]].i = k;
        }
    }
}

void minseqheap_insert(peeridx_t peeridx, seq_t seqbase, struct minseqheap * const h)
{
    peeridx_t i;
#ifndef NDEBUG
    assert(h->ix[peeridx].i == PEERIDX_INVALID);
    for (peeridx_t j = 0; j < h->n; j++) {
        assert(h->hx[j] != peeridx && h->hx[j] < MAX_PEERS_1);
    }
#endif
    h->vs[peeridx] = seqbase;
    i = h->n++;
    while (i > 0 && seq_lt(h->vs[peeridx], h->vs[h->hx[(i-1)/2]])) {
        h->hx[i] = h->hx[(i-1)/2];
        h->ix[h->hx[i]].i = i;
        i = (i-1)/2;
    }
    h->hx[i] = peeridx;
    h->ix[h->hx[i]].i = i;
}

seq_t minseqheap_get_min(struct minseqheap const * const h)
{
    assert (h->n > 0);
    return h->vs[h->hx[0]];
}

seq_t minseqheap_update_seq(peeridx_t peeridx, seq_t seqbase, seq_t seqbase_if_discarded, struct minseqheap * const h)
{
    /* peeridx must be contained in heap and new seqbase must be >= h->vs[peeridx] */
    if (h->ix[peeridx].i == PEERIDX_INVALID || seq_le(seqbase, h->vs[peeridx])) {
        return seqbase_if_discarded;
    } else {
        assert(h->hx[h->ix[peeridx].i] == peeridx);
        h->vs[peeridx] = seqbase;
        minseqheap_heapify(h->ix[peeridx].i, h->n, h->hx, h->ix, h->vs);
        return h->vs[h->hx[0]];
    }
}

int minseqheap_delete(peeridx_t peeridx, struct minseqheap * const h)
{
    /* returns 0 if peeridx not contained in heap; 1 if it is contained */
    const peeridx_t i = h->ix[peeridx].i;
    if (i == PEERIDX_INVALID) {
#ifndef NDEBUG
        for (peeridx_t j = 0; j < h->n; j++) {
            assert(h->hx[j] != peeridx && h->hx[j] < MAX_PEERS_1);
        }
#endif
        return 0;
    } else {
        assert(h->hx[i] == peeridx);
        h->ix[peeridx].i = PEERIDX_INVALID;
        if (h->n > 1) {
            h->hx[i] = h->hx[--h->n];
            h->ix[h->hx[i]].i = i;
            minseqheap_heapify(i, h->n, h->hx, h->ix, h->vs);
        } else {
            h->n = 0;
            h->hx[0] = PEERIDX_INVALID;
        }
        return 1;
    }
}

int minseqheap_isempty(struct minseqheap const * const h)
{
    return h->n == 0;
}

#endif
