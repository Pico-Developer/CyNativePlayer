/*
 * Copyright (c) 2026 ByteDance Ltd. and/or its affiliates
 * SPDX-License-Identifier: LGPL-2.1-only
 */

#include <errno.h>
#include <stdlib.h>
#include "ijksdl/ijksdl_mutex.h"
#include "utils/ncfence/ncfence.h"
#include "utils/nclog/log_util.h"

#define NCResult int
#define NC_SUCC 0
#define NC_EINVAL (-EINVAL)
#define NC_ENOMEM (-ENOMEM)
#define NC_EAGAIN (-EAGAIN)
#define NCFUNC_LABEL(label, id) label##id
#define NCLOGE CYLOGE
#define NCLOGW CYLOGW

struct NCFence {
    SDL_mutex *mutex;
    SDL_cond *cond;
    bool signaled;
};

NCResult ncfence_init(NCFence **fence, bool signaled) {
    if (!fence) return NC_EINVAL;

    NCFence *f = (NCFence *)calloc(1, sizeof(*f));
    if (!f) return NC_ENOMEM;

    NCResult ncr = NC_SUCC;

    if ((f->mutex = SDL_CreateMutex()) == NULL) {
        ncr = NC_EAGAIN;
        NCLOGE("couldn't create muext for fence");
        goto NCFUNC_LABEL(failed, 0);
    }

    if ((f->cond = SDL_CreateCond()) == NULL) {
        ncr = NC_EAGAIN;
        NCLOGE("couldn't create cond for fence");
        goto NCFUNC_LABEL(failed, 1);
    }

    f->signaled = signaled;
    *fence = f;

    return ncr;

NCFUNC_LABEL(failed, 1):
    SDL_DestroyMutex(f->mutex);
NCFUNC_LABEL(failed, 0):
    free(f);
    return ncr;
}

NCResult ncfence_destroy(NCFence **fence) {
    if (!fence || !*fence) return NC_EINVAL;

    SDL_DestroyMutex((*fence)->mutex);
    SDL_DestroyCond((*fence)->cond);
    free(*fence);
    *fence = NULL;
    return NC_SUCC;
}

NCResult ncfence_wait(NCFence *fence) {
    if (!fence) return NC_EINVAL;

    if (SDL_LockMutex(fence->mutex) != 0) {
        NCLOGE("couldn't lock mutex for fence");
        return NC_EAGAIN;
    }

    NCResult ncr = NC_SUCC;

    // use `if` instead of `while` because only need check once
    if (!fence->signaled) {
        if (SDL_CondWait(fence->cond, fence->mutex) != 0) {
            NCLOGE("couldn't wait cond for fence");
            SDL_UnlockMutex(fence->mutex);
            return NC_EAGAIN;
        }
    }

    SDL_UnlockMutex(fence->mutex);
    return ncr;
}

NCResult ncfence_signal(NCFence *fence) {
    if (!fence) return NC_EINVAL;

    if (SDL_LockMutex(fence->mutex) != 0) {
        NCLOGE("couldn't lock mutex for fence");
        return NC_EAGAIN;
    }

    if (fence->signaled) {
        NCLOGW("fence already signaled");
    }
    fence->signaled = true;

    NCResult ncr = NC_SUCC;
    if (SDL_CondSignal(fence->cond) != 0) {
        NCLOGE("couldn't signal cond for fence");
        ncr = NC_EAGAIN;
    }

    SDL_UnlockMutex(fence->mutex);
    return ncr;
}


NCResult ncfence_reset(NCFence *fence) {
    if (!fence) return NC_EINVAL;

    if (SDL_LockMutex(fence->mutex) != 0) {
        NCLOGE("couldn't lock mutex for fence");
        return NC_EAGAIN;
    }

    fence->signaled = false;

    SDL_UnlockMutex(fence->mutex);
    return NC_SUCC;
}
