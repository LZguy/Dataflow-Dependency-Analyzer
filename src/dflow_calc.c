#include "dflow_calc.h"
#include <stdlib.h>     /* malloc, calloc, free */
#include <string.h>     /* memset */

#define REG_COUNT     32
#define INVALID_DEP  (-1)

/*---------- Internal helpers ----------*/

#define MAX(a,b) (( (a) > (b) ) ? (a) : (b))

/* Safe calloc that returns NULL on size==0 as well */
static void *safe_calloc(size_t n, size_t sz)
{
    return (n == 0 || sz == 0) ? NULL : calloc(n, sz);
}

/*---------- Internal context structure ----------*/

typedef struct {
    unsigned int num_insts;          // total instructions in trace
    unsigned int *latency;           // latency per instruction
    int          *depth;             // entry‑to‑issue depth per instruction
    int          *src1_dep, *src2_dep; // instruction index of each dependency
    int           prog_depth;        // critical‑path length (cycles)
} ProgCtxImpl;

/*----------  API implementation  ----------*/

ProgCtx analyzeProg(const unsigned int opsLatency[],
                    const InstInfo   progTrace[],
                    unsigned int     numOfInsts)
{
    // Sanity checks
    if (progTrace == NULL || opsLatency == NULL || numOfInsts == 0)
        return PROG_CTX_NULL;

    // Allocate context
    ProgCtxImpl *ctx = (ProgCtxImpl *)malloc(sizeof(ProgCtxImpl));
    if (!ctx) return PROG_CTX_NULL;

    ctx->num_insts = numOfInsts;
    ctx->latency   = (unsigned int *)safe_calloc(numOfInsts, sizeof(unsigned int));
    ctx->depth     = (int *)safe_calloc(numOfInsts, sizeof(int));
    ctx->src1_dep  = (int *)safe_calloc(numOfInsts, sizeof(int));
    ctx->src2_dep  = (int *)safe_calloc(numOfInsts, sizeof(int));

    if (!ctx->latency || !ctx->depth || !ctx->src1_dep || !ctx->src2_dep) {
        freeProgCtx((ProgCtx)ctx);
        return PROG_CTX_NULL;
    }

    int last_writer[REG_COUNT];
    for (int r = 0; r < REG_COUNT; ++r) last_writer[r] = INVALID_DEP;

    int max_exit_depth = 0;

    for (unsigned int i = 0; i < numOfInsts; ++i) {
        const InstInfo *inst = &progTrace[i];

        /* Record latency for current instruction (default 0 if opcode too large) */
        ctx->latency[i] = opsLatency[inst->opcode];

        /* Dependency lookup */
        int dep1 = (inst->src1Idx < REG_COUNT) ? last_writer[inst->src1Idx] : INVALID_DEP;
        int dep2 = (inst->src2Idx < REG_COUNT) ? last_writer[inst->src2Idx] : INVALID_DEP;
        ctx->src1_dep[i] = dep1;
        ctx->src2_dep[i] = dep2;

        /* Compute depth */
        int depth1 = (dep1 == INVALID_DEP) ? 0 : ctx->depth[dep1] + (int)ctx->latency[dep1];
        int depth2 = (dep2 == INVALID_DEP) ? 0 : ctx->depth[dep2] + (int)ctx->latency[dep2];
        ctx->depth[i] = MAX(depth1, depth2);

        /* Update last writer */
        if (inst->dstIdx < REG_COUNT)
            last_writer[inst->dstIdx] = (int)i;

        /* Track program‑wide critical path */
        int inst_exit = ctx->depth[i] + (int)ctx->latency[i];
        if (inst_exit > max_exit_depth)
            max_exit_depth = inst_exit;
    }

    ctx->prog_depth = max_exit_depth;
    return (ProgCtx)ctx;
}

void freeProgCtx(ProgCtx ctx)
{
    if (ctx == PROG_CTX_NULL) return;
    ProgCtxImpl *p = (ProgCtxImpl *)ctx;
    free(p->latency);
    free(p->depth);
    free(p->src1_dep);
    free(p->src2_dep);
    free(p);
}

int getInstDepth(ProgCtx ctx, unsigned int theInst)
{
    ProgCtxImpl *p = (ProgCtxImpl *)ctx;
    if (!p || theInst >= p->num_insts) return -1;
    return p->depth[theInst];
}

int getInstDeps(ProgCtx ctx, unsigned int theInst,
                int *src1DepInst, int *src2DepInst)
{
    ProgCtxImpl *p = (ProgCtxImpl *)ctx;
    if (!p || theInst >= p->num_insts) return -1;

    if (src1DepInst) *src1DepInst = p->src1_dep[theInst];
    if (src2DepInst) *src2DepInst = p->src2_dep[theInst];
    return 0;
}

int getProgDepth(ProgCtx ctx)
{
    ProgCtxImpl *p = (ProgCtxImpl *)ctx;
    return p ? p->prog_depth : -1;
}

