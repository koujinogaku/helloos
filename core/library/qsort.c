#include "stdlib.h"
#include "memory.h"

#ifdef I_AM_QSORT_R
typedef int              cmp_t(void *, const void *, const void *);
#else
typedef int              cmp_t(const void *, const void *);
#endif
static inline char      *med3(char *, char *, char *, cmp_t *, void *);
static inline void       swapfunc(char *, char *, int, int);

#ifndef min
#define min(a, b)       (a) < (b) ? a : b
#endif
/*
 * Qsort routine from Bentley & McIlroy's "Engineering a Sort Function".
 */
#define swapcode(TYPE, parmi, parmj, n) {               \
         long i = (n) / sizeof (TYPE);                   \
         TYPE *pi = (TYPE *) (parmi);            \
         TYPE *pj = (TYPE *) (parmj);            \
         do {                                            \
                 TYPE    t = *pi;                \
                 *pi++ = *pj;                            \
                 *pj++ = t;                              \
         } while (--i > 0);                              \
 }
 
#define SWAPINIT(a, es) swaptype = ((char *)a - (char *)0) % sizeof(long) || \
         es % sizeof(long) ? 2 : es == sizeof(long)? 0 : 1;

static inline void
swapfunc(a, b, n, swaptype)
        char *a, *b;
        int n, swaptype;
{
        if(swaptype <= 1)
                swapcode(long, a, b, n)
        else
                swapcode(char, a, b, n)
}

#define swap(a, b)                                      \
        if (swaptype == 0) {                            \
                long t = *(long *)(a);                  \
                *(long *)(a) = *(long *)(b);            \
                *(long *)(b) = t;                       \
        } else                                          \
                swapfunc(a, b, es, swaptype)

#define vecswap(a, b, n)        if ((n) > 0) swapfunc(a, b, n, swaptype)

#ifdef I_AM_QSORT_R
#define CMP(t, x, y) (cmp((t), (x), (y)))
#else
#define CMP(t, x, y) (cmp((x), (y)))
#endif

static inline char *
med3(char *a, char *b, char *c, cmp_t *cmp, void *thunk)
{
        return CMP(thunk, a, b) < 0 ?
               (CMP(thunk, b, c) < 0 ? b : (CMP(thunk, a, c) < 0 ? c : a ))
              :(CMP(thunk, b, c) > 0 ? b : (CMP(thunk, a, c) < 0 ? a : c ));
}

#ifdef I_AM_QSORT_R
void
qsort_r(void *a, unsigned int, unsigned int es, void *thunk, cmp_t *cmp)
#else
#define thunk NULL
void
qsort(void *a, unsigned int n, unsigned int es, cmp_t *cmp)
#endif
{
        char *pa, *pb, *pc, *pd, *pl, *pm, *pn;
        unsigned int d, r;
        int cmp_result;
        int swaptype, swap_cnt;

loop:   SWAPINIT(a, es);
        swap_cnt = 0;
        if (n < 7) {
                for (pm = (char *)a + es; pm < (char *)a + n * es; pm += es)
                        for (pl = pm; 
                             pl > (char *)a && CMP(thunk, pl - es, pl) > 0;
                             pl -= es)
                                swap(pl, pl - es);
                return;
        }
        pm = (char *)a + (n / 2) * es;
        if (n > 7) {
                pl = a;
                pn = (char *)a + (n - 1) * es;
                if (n > 40) {
                        d = (n / 8) * es;
                        pl = med3(pl, pl + d, pl + 2 * d, cmp, thunk);
                        pm = med3(pm - d, pm, pm + d, cmp, thunk);
                        pn = med3(pn - 2 * d, pn - d, pn, cmp, thunk);
                }
                pm = med3(pl, pm, pn, cmp, thunk);
        }
        swap(a, pm);
        pa = pb = (char *)a + es;

        pc = pd = (char *)a + (n - 1) * es;
        for (;;) {
                while (pb <= pc && (cmp_result = CMP(thunk, pb, a)) <= 0) {
                        if (cmp_result == 0) {
                                swap_cnt = 1;
                                swap(pa, pb);
                                pa += es;
                        }
                        pb += es;
                }
                while (pb <= pc && (cmp_result = CMP(thunk, pc, a)) >= 0) {
                        if (cmp_result == 0) {
                                swap_cnt = 1;
                                swap(pc, pd);
                                pd -= es;
                        }
                        pc -= es;
                }
                if (pb > pc)
                        break;
                swap(pb, pc);
                swap_cnt = 1;
                pb += es;
                pc -= es;
        }
        if (swap_cnt == 0) {  /* Switch to insertion sort */
                for (pm = (char *)a + es; pm < (char *)a + n * es; pm += es)
                        for (pl = pm; 
                             pl > (char *)a && CMP(thunk, pl - es, pl) > 0;
                             pl -= es)
                                swap(pl, pl - es);
                return;
        }

        pn = (char *)a + n * es;
        r = min(pa - (char *)a, pb - pa);
        vecswap(a, pb - r, r);
        r = min(pd - pc, pn - pd - es);
        vecswap(pb, pn - r, r);
        if ((r = pb - pa) > es)
#ifdef I_AM_QSORT_R
                qsort_r(a, r / es, es, thunk, cmp);
#else
                qsort(a, r / es, es, cmp);
#endif
        if ((r = pd - pc) > es) {
                /* Iterate rather than recurse to save stack space */
                a = pn - r;
                n = r / es;
                goto loop;
        }
/*              qsort(pn - r, r / es, es, cmp);*/
}
