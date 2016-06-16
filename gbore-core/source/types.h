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
#include <stdint.h>
#include <stdbool.h>



typedef uint8_t  gb_opcode_t;
typedef uint8_t  byte_t;
typedef uint8_t  gb_word_t;
typedef uint16_t gb_dword_t;
typedef uint16_t gb_addr_t;


typedef union {
    uint16_t dw;
    struct {
        uint8_t w0;
        uint8_t w1;
    }w;
} gb_dword_u;

typedef enum {
    gb_error_success = 0x0,
    gb_error_fatal = 0xC8000000,
    gb_error_failure = 0xC8000001,
    gb_error_OS_failure = 0xC8000002,
    gb_error_missing_parameter = 0xC80000E0,
    gb_error_invalid_parameter = 0xC80000F0,

} gb_error_t;

enum gb_meta_device {
    gb_dev_Uninitialized,
    gb_dev_DMG,      // Original GameBoy
    gb_dev_GBP,     // GameBoy Pocket
    gb_dev_SGB,     // Super GameBoy
    gb_dev_GBC,     // GameBoy Color
};

typedef enum{
    gb_tristate_false,
    gb_tristate_true,
    gb_tristate_none,

}gb_tristate;

typedef struct {
    byte_t * buffer;
    size_t  size;
} gb_bytebuf_t;



// Generic opcode handler function signature
// uint8_t - the interpreted opcode
// uint16_t - dword following the opcode 
// Returns: number of spent CPU cycles
typedef byte_t (*gb_opcode_handler_pfn)(uint8_t, uint16_t);  


// Generic CPU instruction descriptor (w/handler)
typedef struct gb_instruction_s {
    gb_opcode_handler_pfn handler;
    byte_t           size; 
    byte_t           padding[3];
} gb_instruction_s;


//
// Structure used for matching opcodes with their handlers.
// Usually one handler matches many opcodes, see opcodes.inc
//
typedef struct {

    gb_word_t r;    //r - relevant bits, mask used to filter opcode signature
    gb_word_t m;    //m - match bits, mask used to identify opcode
    gb_word_t n;    //n - number of opcodes matching the signature

} gb_instruction_bits_s;

//
//typedef void* lib_array_t;
//
//lib_array_t lib_array_init(size_t item_size, size_t array_size);
//errno_t     lib_array_insert(lib_array_t arr, void* item_ptr);
//errno_t     lib_array_clear(lib_array_t arr, void* item_ptr);


