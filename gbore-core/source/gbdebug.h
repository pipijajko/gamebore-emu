#pragma once
#include <stdbool.h>
#include <stdlib.h> //abort
typedef struct { void* unused; } gb_debugdata_h; //opaque handle



#ifdef GBDEBUG
struct gb_machine_s;

errno_t gbdbg_initialize(struct gb_machine_s *instance, gb_debugdata_h *handle);
void    gbdbg_destroy(gb_debugdata_h handle);
void    gbdbg_CPU_trace(gb_debugdata_h handle, gb_word_t op, gb_dword_t d16, gb_dword_t progcount);
void    gbdbg_CPU_dumpregs(gb_debugdata_h handle);
int     gdbg_trace(gb_debugdata_h handle, char *fmtstr, ...);
void    gbdbg_mute_print(gb_debugdata_h handle, bool set_mute);



#else
#define gbdbg_initialize(...)
#define gbdbg_destroy(...)
#define gbdbg_CPU_trace(...)
#define gbdbg_CPU_dumpregs(...)
#define gbdbg_mute_print(...);
#endif



