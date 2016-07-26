#include "types.h"
#include "gamebore.h"



void gbdisp_putpixel(gbdisplay_h handle, uint8_t x, uint8_t y, gb_color_s c) {
    UNUSED(handle, x, y, c);
}
void gbdisp_buffer_set_ready(gbdisplay_h handle) {
    UNUSED(handle);
}
bool gbdisp_is_buffer_ready(gbdisplay_h handle) {
    UNUSED(handle);
    return false;
}
