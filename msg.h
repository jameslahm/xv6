#ifndef __MSG_H__
#define __MSG_H__

#define MSG_MOUSE_MOVE 1
#define MSG_MOUSE_DOWN 2
#define MSG_MOUSE_UP   3
#define MSG_MOUSE_LEFT_CLICK 4
#define MSG_MOUSE_RIGHT_CLICK 5
#define MSG_MOUSE_DBCLICK 6


typedef struct {
    int msg_type;
    int params[10];
} Message;

#endif