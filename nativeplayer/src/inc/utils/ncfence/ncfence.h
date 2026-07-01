/*
 * Copyright (c) 2026 ByteDance Ltd. and/or its affiliates
 * SPDX-License-Identifier: LGPL-2.1-only
 */

#ifndef INC_UTILS_NCFENCE_NCFENCE_H
#define INC_UTILS_NCFENCE_NCFENCE_H

#include <stdbool.h>

typedef struct NCFence NCFence;

int ncfence_init(NCFence **fence, bool signaled);
int ncfence_destroy(NCFence **fence);
int ncfence_wait(NCFence *fence);
int ncfence_signal(NCFence *fence);
int ncfence_reset(NCFence *fence);

#endif
