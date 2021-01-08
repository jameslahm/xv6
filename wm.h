#ifndef __WM_H__
#define __WM_H__

#include "msg.h"

#define MSG_BUF_SIZE 50
#define MAX_TITLE_LEN 50

typedef struct WinRect
{
	int xmin, xmax, ymin, ymax;
} WinRect;

typedef struct MsgBuf
{
	Message data[MSG_BUF_SIZE];
	int front, rear, cnt;
} MsgBuf;

typedef struct Window
{
	WinRect contents;
	WinRect titlebar;
	MsgBuf buf;
	struct RGB *content_buf;
	int alwaysfront;
	char title[MAX_TITLE_LEN];
} Window;

#endif
