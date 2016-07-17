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
#include <stdbool.h>
#include <stdint.h>
#include <assert.h>

#include "gamebore.h" 
#include "input.h"

void gb_INPUT_initialize(gb_keypad_s * const kp) {
    kp->direction_keys = 0xCF;
    kp->button_keys    = 0xCF;
    kp->is_initialized = true;
}

void gb_INPUT_press_key(gb_INPUT_key_e const key, bool const is_pressed) {

    gb_keypad_s * const kp = &g_GB.keypad;

    switch (key) {
    case gb_INPUT_right:
        BIT_SETCLR_IF(!is_pressed, kp->direction_keys, GB_KEY_RIGHT);
        break;
    case gb_INPUT_left:
        BIT_SETCLR_IF(!is_pressed, kp->direction_keys, GB_KEY_LEFT);
        break;
    case gb_INPUT_up:
        BIT_SETCLR_IF(!is_pressed, kp->direction_keys, GB_KEY_UP);
        break;
    case gb_INPUT_down:
        BIT_SETCLR_IF(!is_pressed, kp->direction_keys, GB_KEY_DOWN);
        break;
    case gb_INPUT_A:
        BIT_SETCLR_IF(!is_pressed, kp->button_keys, GB_KEY_A);
        break;
    case gb_INPUT_B:
        BIT_SETCLR_IF(!is_pressed, kp->button_keys, GB_KEY_B);
        break;
    case gb_INPUT_select:
        BIT_SETCLR_IF(!is_pressed, kp->button_keys, GB_KEY_SELECT);
        break;
    case gb_INPUT_start:
        BIT_SETCLR_IF(!is_pressed, kp->button_keys, GB_KEY_START);
        break;

    default:
        gdbg_trace(g_GB.dbg, "Invalid key!\n");
        break;
    }
}


void gb_INPUT_step() {

    gb_keypad_s const * const kp = &g_GB.keypad;

    gb_word_t * const P1_JOYP        = GB_MMU_ACCESS_INTERNAL(GB_IO_P1_JOYP);
    gb_word_t  const  requested_keys = (*P1_JOYP) & (GB_KEY_READ_BUTTONS | GB_KEY_READ_DIRECTION);

    // Game decides which keys it wants to read by setting bit 0x10 or 0x20 in P1 JOYP
    // INPUT controller will respond by setting proper key line bits in 0xF

    switch (requested_keys) {
    case 0: //nothing was requested
        return;

    case GB_KEY_READ_BUTTONS:
        *P1_JOYP = kp->button_keys;
        gb_INTERRUPT_request(GB_INT_FLAG_KEYPAD);
        break;

    case GB_KEY_READ_DIRECTION:
        *P1_JOYP = kp->direction_keys;
        gb_INTERRUPT_request(GB_INT_FLAG_KEYPAD);
        break;

    default:
        //gdbg_trace(g_GB.dbg, "Invalid Read selector!\n");
        return;
    }
    //assert(((*P1_JOYP) & (0xF)) == (*P1_JOYP));
}
