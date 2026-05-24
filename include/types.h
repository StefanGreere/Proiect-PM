#ifndef TYPES_H
#define TYPES_H

typedef enum {
    COLOR_UNKNOWN = 0,
    COLOR_RED,
    COLOR_GREEN,
    COLOR_BLUE
} color_t;

typedef enum {
    MODE_NONE = 0,
    MODE_SIMPLE_DETECTION,
    MODE_SCAN_COLOR
} mode_t;

#endif
