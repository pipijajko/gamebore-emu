#pragma once

/*
Include this everywhere you need stuff.
Dont include manually - easy to fuck up.
*/
#define GBDEBUG

#include <stdio.h>
#include <stdarg.h> //va_start et al
#include <stddef.h>

#include "types.h"
#include "helpers.h"
#include "cart.h"
#include "cpu.h"
#include "display.h"
#include "gbdebug.h"
#include "memory.h"
#include "interrupts.h"


typedef struct gb_machine_s {
    
    
    gb_instruction_s    ops[GB_OPCODES_N];
    gb_memory_unit_s    m; // memory
    gb_cpu_registers_s  r; // registers
    gb_interrupt_data_s interrupts; // interrupts

    gb_debugdata_h          dbg; //debug module handle
    enum gb_meta_device      gb_model;

} gb_machine_s;

extern struct gb_machine_s g_GB;

//
// MMU.H needs gb_machine_s to be fully defined for inlining
//




void gb_initialize();
void gb_emulate(void *cart_data, size_t cart_data_size);
void gb_destroy();
