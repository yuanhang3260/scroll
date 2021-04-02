#ifndef MONITOR_H
#define MONITOR_H

#include "common/common.h"

#define COLOR_BLACK 0
#define COLOR_BLUE 1
#define COLOR_GREEN 2
#define COLOR_CYAN 3
#define COLOR_RED 4
#define COLOR_FUCHSINE 5
#define COLOR_BROWN 6
#define COLOR_WHITE 7

#define COLOR_LIGHT_BLACK 8
#define COLOR_LIGHT_BLUE 9
#define COLOR_LIGHT_GREEN 10
#define COLOR_LIGHT_CYAN 11
#define COLOR_LIGHT_RED 12
#define COLOR_LIGHT_FUCHSINE 13
#define COLOR_LIGHT_BROWN 14
#define COLOR_LIGHT_WHITE 15

// Write a single character out to the screen.
void monitor_put(char c);

void monitor_put_with_color(char c, uint8 color);

// Output a null-terminated ASCII string to the monitor.
void monitor_write(char *c);

void monitor_write_with_color(char *c, uint8 color);

// Output a hex number.
void monitor_write_hex(uint32 n);

// Output a decimal number.
void monitor_write_dec(uint32 n);

// Clear the screen to all black.
void monitor_clear();

#endif // MONITOR_H
