#pragma once

// INPUT IO PORTS
#define GB_IO_P1_JOYP 0xFF00 //Register for reading joypad info



#define GB_KEY_PRESSED true
#define GB_KEY_RELEASED false


//define bit mask
#define GB_KEY_RIGHT 0x01
#define GB_KEY_LEFT  0x02
#define GB_KEY_UP    0x04
#define GB_KEY_DOWN  0x08

#define GB_KEY_A      0x01
#define GB_KEY_B      0x02
#define GB_KEY_SELECT 0x04
#define GB_KEY_START  0x08


#define GB_KEY_READ_BUTTONS     0x10
#define GB_KEY_READ_DIRECTION   0x20



typedef struct gb_keypad_s
{
    int8_t button_keys;
    int8_t direction_keys;
    bool   is_initialized;

} gb_keypad_s;


typedef enum gb_INPUT_key_e
{
    gb_INPUT_none,
    gb_INPUT_right,
    gb_INPUT_left,
    gb_INPUT_up,
    gb_INPUT_down,
    gb_INPUT_A,
    gb_INPUT_B,
    gb_INPUT_start,
    gb_INPUT_select,
} gb_INPUT_key_e;


void gb_INPUT_initialize(gb_keypad_s * const kp);
void gb_INPUT_press_key(gb_INPUT_key_e const key, bool const is_pressed);
void gb_INPUT_step(void);




typedef void (gb_input_handler_evt)(gb_INPUT_key_e const key, bool const is_pressed);
typedef gb_input_handler_evt *gb_input_handler_pfn;

