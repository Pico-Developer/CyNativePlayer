
#include "backtrace.h"

_Unwind_Reason_Code unwind_cb(struct _Unwind_Context *ctx, void *arg) {
    BacktraceState *state = (BacktraceState *)arg;
    uintptr_t pc = _Unwind_GetIP(ctx);
    if (pc) {
        if (state->current == state->end) {
            return _URC_END_OF_STACK;
        } else {
            *state->current++ = (void *)pc;
        }
    }
    return _URC_NO_REASON;
}

void capture_backtrace(void *backtrace[], int max_depth) {
    BacktraceState state = {
        .current = backtrace,
        .end = backtrace + max_depth
    };
    _Unwind_Backtrace(unwind_cb, &state);
}

BacktraceInfo backtrace_info(void *addr) {
    const char *sym = "no_found";
    const char *fsym = "no_found";

    Dl_info info;
    if (dladdr(addr, &info) && info.dli_sname) {
        sym = info.dli_sname;
        fsym = info.dli_fname;
    }

    void *r_a = (void *)(addr - info.dli_fbase);

    BacktraceInfo binfo = {
        .sym = sym,
        .f_sym = fsym,
        .relative_addr = r_a
    };

    return binfo;
}
