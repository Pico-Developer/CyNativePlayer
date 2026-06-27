#ifndef BACKTRACE_H
#define BACKTRACE_H
#include <unwind.h>

#include <dlfcn.h>

typedef struct {
    void **current;
    void **end;
} BacktraceState;

typedef struct {
    const char *sym;
    const char *f_sym;
    void *relative_addr;
} BacktraceInfo;

_Unwind_Reason_Code unwind_cb(struct _Unwind_Context *ctx, void *arg);

void capture_backtrace(void *backtrace[], int max_depth);

BacktraceInfo backtrace_info(void *addr);

#endif

