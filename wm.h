#ifndef __WM_H__
#define __WM_H__

#include "msg.h"
#include "gui.h"
#include "defs.h"
#include "spinlock.h"
#include "proc.h"

#define MSG_BUF_SIZE 50
#define MAX_TITLE_LEN 50
#define MAX_WINDOW_CNT 50
#define MOUSE_SPEED_X 0.6f;
#define MOUSE_SPEED_Y -0.6f;

typedef struct {
    int xmin,xmax,ymin,ymax;
} Rect;

typedef struct {
    Message msgs[MSG_BUF_SIZE];
    int front,rear,cnt;
} Message_Buf;

typedef struct {
    Rect contents;
    Rect titlebar;
    Message_Buf buf;
    RGB* content_buf;
    int alwaysfront;
    char title[MAX_TITLE_LEN];
} Window;

typedef struct {
    struct proc* proc;
    Window wnd;
    int next,prev;
} WindowList;

typedef struct {
    float x;
    float y;
} Point;

#endif