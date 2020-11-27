#include "wm.h"

static int windowlisthead, emptyhead;
static int desktopHandler = -100;
static int focus;
static int frontcnt;
static int clickedOnTitle, clickedOnContent;
static Point wm_mouse_pos;
static Point wm_mouse_last_pos;

struct spinlock wmlock;
static WindowList windowlist[MAX_WINDOW_CNT];

void wm_init()
{
    windowlisthead = -1;
    emptyhead = 0;
    for (int i = 0; i < MAX_WINDOW_CNT; i++)
    {
        windowlist[i].next = i + 1;
        windowlist[i].prev = i - 1;
    }
    windowlist[MAX_WINDOW_CNT].next = -1;

    wm_mouse_pos.x = SCREEN_WIDTH / 2;
    wm_mouse_pos.y = SCREEN_HEIGHT / 2;

    wm_mouse_last_pos = wm_mouse_pos;

    frontcnt = 0;
    focus = -1;
    clickedOnContent = clickedOnTitle = 0;

    initlock(&wmlock, "wmlock");
}

void addToListHead(int *head, int idx)
{
    windowlist[idx].prev = -1;
    windowlist[idx].next = *head;
    if (*head != -1)
    {
        windowlist[*head].prev = idx;
    }
    *head = idx;
}

void removeFromList(int *head, int idx)
{
    if (*head == idx)
        *head = windowlist[*head].next;
    if (windowlist[idx].prev != -1)
        windowlist[windowlist[idx].prev].next = windowlist[idx].next;
    if (windowlist[idx].next != -1)
        windowlist[windowlist[idx].next].prev = windowlist[idx].prev;
}

void initqueue(Message_Buf *buf)
{
    buf->front = 0;
    buf->rear = 0;
    buf->cnt = 0;
}

void createRectBySize(Rect *rect, int xmin, int ymin, int width, int height)
{
    rect->xmin = xmin;
    rect->xmax = xmin + width;
    rect->ymin = ymin;
    rect->ymax = ymin + height;
}

void drawWindowBar(RGB *dst, Window *wnd, RGBA barcolor)
{
    RGBA closecolor, txtcolor;
    closecolor.R = 200;
    closecolor.G = 50;
    closecolor.B = 10;
    closecolor.A = 255;
    txtcolor.R = txtcolor.G = txtcolor.B = txtcolor.A = 255;
    drawRectByCoord(dst, wnd->titlebar.xmin, wnd->titlebar.ymin, wnd->titlebar.xmax - 30, wnd->titlebar.ymax, barcolor);
    drawRectByCoord(dst, wnd->titlebar.xmax - 30, wnd->titlebar.ymin, wnd->titlebar.xmax - 3, wnd->titlebar.ymax, closecolor);
    drawRectByCoord(dst, wnd->titlebar.xmax - 3, wnd->titlebar.ymin, wnd->titlebar.xmax, wnd->contents.ymax + 3, barcolor);
    drawRectByCoord(dst, wnd->titlebar.xmin, wnd->contents.ymin, wnd->contents.xmin, wnd->contents.ymax + 3, barcolor);
    drawRectByCoord(dst, wnd->contents.xmin, wnd->contents.ymax, wnd->contents.xmax, wnd->contents.ymax + 3, barcolor);
    drawString(dst, wnd->titlebar.xmin + 5, wnd->titlebar.ymin + 3, wnd->title, txtcolor);
}

// 绘制窗口
void drawWindow(int layer, int handler, int refresh)
{
    Window *wnd = &windowlist[handler].wnd;
    RGBA barcolor, wndcolor;
    barcolor.R = 170;
    barcolor.G = 150;
    barcolor.B = 100;
    barcolor.A = 255;

    if (layer == 2)
    {
        barcolor.R = barcolor.G = barcolor.B = 140;
    }

    wndcolor.R = 255;
    wndcolor.G = 255;
    wndcolor.B = 255;
    wndcolor.A = 255;

    RGB *dst;
    if (layer == 2)
    {
        dst == screen_buf2;
    }
    else if (layer == 1)
    {
        dst = screen_buf1;
    }
    else
    {
        dst = screen;
    }

    if (handler != desktopHandler)
    {
        drawWindowBar(dst, wnd, barcolor);
    }

    switchuvm(windowlist[handler].proc);
    draw24ImagePart(dst, wnd->content_buf, wnd->contents.xmin, wnd->contents.ymin,
                    wnd->contents.xmax - wnd->contents.xmin, wnd->contents.ymax - wnd->contents.ymin,
                    0, 0, wnd->contents.xmax - wnd->contents.xmin, wnd->contents.ymax - wnd->contents.ymin);

    if (proc == 0)
    {
        switchkvm();
    }
    else
    {
        switchuvm(proc);
    }

    if (refresh)
    {
        if (layer >= 2)
            clearRectByCoord(screen_buf1, screen_buf2, wnd->titlebar.xmin, wnd->titlebar.ymin, wnd->titlebar.xmax, wnd->contents.ymax + 3);
        if (layer >= 1)
            clearRectByCoord(screen, screen_buf1, wnd->titlebar.xmin, wnd->titlebar.ymin, wnd->titlebar.xmax, wnd->contents.ymax + 3);
    }
}

void refreshWindowScreen(int layer, int handler)
{
    Window *wnd = &windowlist[handler].wnd;
    RGB *dst;
    if (layer == 2)
    {
        dst = screen_buf2;
    }
    else if (layer == 1)
    {
        dst = screen_buf1;
    }
    else
    {
        dst = screen;
    }
    clearRectByCoord(screen, dst, wnd->titlebar.xmin, wnd->titlebar.ymin, wnd->titlebar.xmax, wnd->contents.ymax + 3);
}

void focusWindow(int handler)
{
    if (frontcnt)
    {
        return;
    }
    if (handler != desktopHandler)
    {
        removeFromList(&windowlisthead, handler);
        addToListHead(&windowlisthead, handler);

        int p, q;
        for (p = windowlisthead; p != -1; p = windowlist[p].next)
        {
            q = p;
        };
        for (p = q; p != windowlisthead; p = windowlist[p].prev)
        {
            drawWindow(2, p, 0);
        }
        clearRect(screen_buf1, screen_buf2, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
        drawWindow(1, windowlisthead, 0);
        if (focus != desktopHandler && focus != -1)
        {
            clearRectByCoord(screen, screen_buf1,
                             min(windowlist[focus].wnd.titlebar.xmin, windowlist[handler].wnd.titlebar.xmin),
                             min(windowlist[focus].wnd.titlebar.ymin, windowlist[handler].wnd.titlebar.ymin),
                             max(windowlist[focus].wnd.titlebar.xmax, windowlist[handler].wnd.titlebar.xmax),
                             max(windowlist[focus].wnd.contents.ymax, windowlist[handler].wnd.contents.ymax) + 3);
        }
        else
        {
            refreshWindowScreen(1, handler);
        }
        drawMouse(screen, 0, wm_mouse_pos.x, wm_mouse_pos.y);
    }
    else if (focus != -1)
    {
        RGBA gray;
        gray.A = 255;
        gray.R = gray.G = gray.B = 140;
        drawWindowBar(screen_buf1, &windowlist[focus].wnd, gray);
        drawWindowBar(screen, &windowlist[focus].wnd, gray);
    }
    else
    {
        drawWindow(2, desktopHandler, 1);
    }

    focus = handler;
}

int createWindow(int width, int height, const char *title, RGB *buf, int alwaysfront)
{
    if (emptyhead == -1)
    {
        return -1;
    }
    uint title_len = strlen(title);
    if (title_len >= MAX_TITLE_LEN)
    {
        return -1;
    }
    if (alwaysfront && frontcnt)
    {
        return -1;
    }

    acquire(&wmlock);

    int idx = emptyhead;
    removeFromList(&emptyhead, idx);
    addToListHead(&windowlisthead, idx);

    // 初始化消息队列
    initqueue(&windowlist[idx].wnd.buf);

    int offsetX = (100 + idx * 47) % (SCREEN_WIDTH - 130) + 30;
    int offsetY = (100 + idx * 33) % (SCREEN_HEIGHT - 130) + 30;

    // 桌面初始化
    if (title_len == 0 && desktopHandler == -100)
    {
        desktopHandler = idx;
        createRectBySize(&windowlist[idx].wnd.contents, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
        createRectBySize(&windowlist[idx].wnd.titlebar, 0, 0, SCREEN_WIDTH, 0);
    }
    else
    {
        createRectBySize(&windowlist[idx].wnd.contents, offsetX, offsetY, width, height);
        createRectBySize(&windowlist[idx].wnd.titlebar, offsetX - 3, offsetY - 20, width + 6, 20);
        memmove(windowlist[idx].wnd.title, title, title_len);
    }
    windowlist[idx].proc = proc;
    windowlist[idx].wnd.alwaysfront = alwaysfront;
    windowlist[idx].wnd.content_buf = buf;

    focusWindow(idx);
    if (frontcnt)
    {
        drawWindow(2, idx, 1);
        drawMouse(screen, 0, wm_mouse_pos.x, wm_mouse_pos.y);
    }

    if (alwaysfront)
        ++frontcnt;

    release(&wmlock);
    return idx;
}

void getWindowRect(int hander, Rect *res)
{
    Window *wnd = &windowlist[hander].wnd;
    res->xmin = wnd->titlebar.xmin;
    res->ymin = wnd->titlebar.ymin;
    res->xmax = wnd->titlebar.xmax;
    res->ymax = wnd->contents.ymax + 3;
}

void moveRect(Rect *rect, int dx, int dy)
{
    rect->xmin += dx;
    rect->xmax += dx;
    rect->ymin += dy;
    rect->ymax += dy;
}

void vmMoveFocusWindow(int dx, int dy)
{
    Rect winrect;
    getWindowRect(focus, &winrect);
    clearRectByCoord(screen_buf1, screen_buf2, winrect.xmin, winrect.ymin, winrect.xmax, winrect.ymax);
    moveRect(&windowlist[focus].wnd.contents, dx, dy);
    moveRect(&windowlist[focus].wnd.titlebar, dx, dy);
    drawWindow(1, focus, 0);
    if (dx > 0)
    {
        winrect.xmax += dx;
    }
    else
    {
        winrect.xmin += dx;
    }
    if (dy > 0)
    {
        winrect.ymax += dy;
    }
    else
    {
        winrect.ymin += dy;
    }
    clearRectByCoord(screen, screen_buf1, winrect.xmin, winrect.ymin, winrect.xmax, winrect.ymax);
    drawMouse(screen, 0, wm_mouse_pos.x, wm_mouse_pos.y);
}

int isInRect(Rect *rect, int x, int y)
{
    return (x >= rect->xmin && x <= rect->xmax && y >= rect->ymin && y <= rect->ymax);
}

int enqueue(Message_Buf *buf, Message *msg)
{
    if (buf->cnt >= MSG_BUF_SIZE)
    {
        return 1;
    }
    buf->cnt++;
    buf->msgs[buf->rear] = *msg;
    buf->rear++;
    if (buf->rear >= MSG_BUF_SIZE)
    {
        buf->rear = 0;
    }
    return 0;
}

int dequeue(Message_Buf *buf, Message *result)
{
    if (buf->cnt == 0)
    {
        return 1;
    }
    buf->cnt--;
    *result = buf->msgs[buf->front];
    buf->front++;
    if (buf->front >= MSG_BUF_SIZE)
    {
        buf->front = 0;
    }
    return 0;
}

void dispatchMessage(int handler, Message *msg)
{
    enqueue(&windowlist[handler].wnd.buf, msg);
}

void handle_message(Message *msg)
{
    acquire(&wmlock);
    Message newmsg;
    int p;
    switch (msg->msg_type)
    {
    case MSG_MOUSE_MOVE:
    {
        wm_mouse_last_pos = wm_mouse_pos;
        wm_mouse_pos.x += msg->params[0] * MOUSE_SPEED_X;
        wm_mouse_pos.y += msg->params[1] * MOUSE_SPEED_Y;
        if (wm_mouse_pos.x > SCREEN_WIDTH)
        {
            wm_mouse_pos.x = SCREEN_WIDTH;
        }
        if (wm_mouse_pos.y > SCREEN_HEIGHT)
        {
            wm_mouse_pos.y = SCREEN_HEIGHT;
        }
        if (wm_mouse_pos.x < 0)
        {
            wm_mouse_pos.x = 0;
        }
        if (wm_mouse_pos.y < 0)
        {
            wm_mouse_pos.y = 0;
        }
        clearMouse(screen, screen_buf1, wm_mouse_last_pos.x, wm_mouse_last_pos.y);
        drawMouse(screen, 0, wm_mouse_pos.x, wm_mouse_pos.y);

        if (clickedOnTitle)
        {
            wmMoveFocusWindow(wm_mouse_pos.x - wm_mouse_last_pos.x, wm_mouse_pos.y - wm_mouse_last_pos.y);
        }
        break;
    }
    case MSG_MOUSE_DOWN:
    {
        if (frontcnt == 0)
        {
            for (p = windowlisthead; p != -1; p = windowlist[p].next)
            {
                if (isInRect(&windowlist[p].wnd.titlebar, wm_mouse_pos.x, wm_mouse_pos.y) ||
                    isInRect(&windowlist[p].wnd.contents, wm_mouse_pos.x, wm_mouse_pos.y))
                {
                    if (p != focus)
                        focusWindow(p);
                    break;
                }
            }
        }
        if (isInRect(&windowlist[focus].wnd.contents, wm_mouse_pos.x, wm_mouse_pos.y))
        {
            clickedOnContent = 1;
            newmsg = *msg;
            // 转换坐标到窗口
            newmsg.params[0] = wm_mouse_pos.x - windowlist[focus].wnd.contents.xmin;
            newmsg.params[1] = wm_mouse_pos.y - windowlist[focus].wnd.contents.ymin;
            newmsg.params[2] = msg->params[0];
            dispatchMessage(focus, &newmsg);
        }
        // close window
        else if (wm_mouse_pos.x + 30 > windowlist[focus].wnd.titlebar.xmax)
        {
            newmsg.msg_type = MSG_WINDOW_CLOSE;
            dispatchMessage(focus, &newmsg);
        }
        else // titlebar
        {
            clickedOnTitle = 1;
        }
        break;
    }
    case MSG_MOUSE_LEFT_CLICK:
    {
        if(clickedOnContent){
            clickedOnContent = 0;
            newmsg = *msg;
            newmsg.params[0] = wm_mouse_pos.x - windowlist[focus].wnd.contents.xmin;
	        newmsg.params[1] = wm_mouse_pos.y - windowlist[focus].wnd.contents.ymin;
	        newmsg.params[2] = msg->params[0];
		    dispatchMessage(focus, &newmsg);
        }
        break;
    }
    case MSG_MOUSE_RIGHT_CLICK:
    {
        if (clickedOnContent)
	    {
	        clickedOnContent = 0;
		    newmsg = *msg;
	        newmsg.params[0] = wm_mouse_pos.x - windowlist[focus].wnd.contents.xmin;
	        newmsg.params[1] = wm_mouse_pos.y - windowlist[focus].wnd.contents.ymin;
	        newmsg.params[2] = msg->params[0];
		    dispatchMessage(focus, &newmsg);
	    }
        break;
    }
    case MSG_MOUSE_DBCLICK:{
        if (clickedOnContent)
	    {
	        clickedOnContent = 0;
		    newmsg = *msg;
	        newmsg.params[0] = wm_mouse_pos.x - windowlist[focus].wnd.contents.xmin;
	        newmsg.params[1] = wm_mouse_pos.y - windowlist[focus].wnd.contents.ymin;
	        newmsg.params[2] = msg->params[0];
		    dispatchMessage(focus, &newmsg);
	    }
        break;
    }
    case MSG_MOUSE_UP:{
        if (clickedOnContent)
		{
		    clickedOnContent = 0;
		    newmsg = *msg;
	        newmsg.params[0] = wm_mouse_pos.x - windowlist[focus].wnd.contents.xmin;
	        newmsg.params[1] = wm_mouse_pos.y - windowlist[focus].wnd.contents.ymin;
	        newmsg.params[2] = msg->params[0];
		    dispatchMessage(focus, &newmsg);
		}
		else if (clickedOnTitle)
		{
		    clickedOnTitle = 0;
		}
        break;
    }
    
    }
}