
#include <stdint.h>
#include <stddef.h>
#include "libavutil/dict.h"

uintptr_t av_dict_strtoptr(char * value) {
   uintptr_t ptr = NULL;
   char *next = NULL;
   if(!value || value[0] !='0' || (value[1]|0x20)!='x') {
       return NULL;
   }
   ptr = strtoull(value, &next, 16);
   if (next == value) {
       return NULL;
   }
   return ptr;
}

int av_dict_set_intptr(AVDictionary **pm, const char *key, uintptr_t value,
                int flags)
{
    char valuestr[22];
    snprintf(valuestr, sizeof(valuestr), "%p", value);
    flags &= ~AV_DICT_DONT_STRDUP_VAL;
    return av_dict_set(pm, key, valuestr, flags);
}
