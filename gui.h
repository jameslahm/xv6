#ifndef __GUI_H__
#define __GUI_H__

#include "types.h"
#include "memlayout.h"
#include "user.h"
#include "defs.h"
#include "character.h"

ushort SCREEN_WIDTH;
ushort SCREEN_HEIGHT;
int SCREEN_SIZE;

typedef struct {
    uchar B;
    uchar G;
    uchar R;

} RGB;

typedef struct {
    uchar B;
    uchar G;
    uchar R;
    uchar A;
} RGBA;

#endif