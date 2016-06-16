/*
Copyright 2016 Maciej Kurczewski

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/
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
#include "input.h"
#include "display.h"
#include "gbdebug.h"
#include "memory.h"
#include "interrupts.h"


typedef struct gb_machine_s {
    
    
    gb_instruction_s    ops[GB_OPCODES_N];
    gb_memory_unit_s    m; // memory
    gb_cpu_registers_s  r; // registers
    gb_interrupt_data_s interrupts; // interrupts
    gb_keypad_s         keypad;
    gb_debugdata_h      dbg; //debug module handle
    enum gb_meta_device gb_model;

} gb_machine_s;

extern struct gb_machine_s g_GB;

//
// MMU.H needs gb_machine_s to be fully defined for inlining
//


void gb_initialize(void const* const cart_data, size_t const cart_data_size);
void gb_machine_step(gbdisplay_h disp);
void gb_destroy();
