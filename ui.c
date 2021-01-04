#include "types.h"
#include "stat.h"
#include "color.h"
#include "msg.h"
#include "user.h"
#include "fcntl.h"
#include "ui.h"
#include "character.h"
#include "fs.h"
// #include "kbd.h"
#define KEY_HOME 0xE0
#define KEY_END 0xE1
#define KEY_UP 0xE2
#define KEY_DN 0xE3
#define KEY_LF 0xE4
#define KEY_RT 0xE5
#define KEY_PGUP 0xE6
#define KEY_PGDN 0xE7
#define KEY_INS 0xE8
#define KEY_DEL 0xE9

void drawImageWidget(window *win, int index);
void drawLabelWidget(window *win, int index);
void drawButtonWidget(window *win, int index);
void drawInputWidget(window *win, int index);
void drawTextAreaWidget(window *win, int index);
void drawFileListWidget(window *win, int index);

void fileListDoubleClickHandler(window *win, int index, message *msg);
void textAreaKeyDownHandler(window *win, int index, message *msg);
void generateHighlightRGB(Widget * w);

char file_image_path[FILE_TYPE_NUM][MAX_SHORT_STRLEN] = {
    "txt.bmp",
    "pic.bmp",
    "exec.bmp",
    "folder.bmp",
    "unknown.bmp"};

void UI_createWindow(window *win, const char *title, int alwaysfront)
{
    if (win->width > MAX_WIDTH || win->height > MAX_HEIGHT)
    {
        win->handler = -1;
        return;
    }
    win->window_buf = malloc(win->width * win->height * 3);
    if (!win->window_buf)
    {
        win->handler = -1;
        return;
    }
    memset(win->window_buf, 255, win->height * win->width * 3);
    win->handler = createwindow(win->width, win->height, title, win->window_buf, alwaysfront);
}

void freeWidget(Widget *widget)
{
    switch (widget->type)
    {
    case IMAGE:
        free(widget->context.image);
        break;
    case LABEL:
        free(widget->context.label);
        break;
    case BUTTON:
        free(widget->context.button);
        break;
    case INPUT:
        free(widget->context.input);
        break;
    case TEXT_AREA:
        free(widget->context.textArea);
        break;
    default:
        free(widget->context.fileList);
        break;
    }
}

void UI_destroyWindow(window *win)
{
    free(win->window_buf);
    int i;
    for (i = 0; i < win->widget_number; i++)
    {
        if (win->widgets[i].type == FILE_LIST)
        {
            IconView *p;
            IconView *temp;
            for (p = win->widgets[i].context.fileList->file_list; p; p = temp)
            {
                temp = p->next;
                free(p);
            }
            int j;
            for (j = 0; j < FILE_TYPE_NUM; j++)
            {
                free(win->widgets[i].context.fileList->image[j]);
            }
        }
        freeWidget(&win->widgets[i]);
    }
    destroywindow(win->handler);
}

// system call
void updatePartWindow(window *win, int x, int y, int w, int h)
{
    updatewindow(win->handler, x, y, w, h);
}

void setWidgetSize(Widget *widget, int x, int y, int w, int h)
{
    widget->size.x = x;
    widget->size.y = y;
    widget->size.height = h;
    widget->size.width = w;
}

int addImageWidget(window *win, RGB *image, int x, int y, int w, int h)
{
    if (win->widget_number >= MAX_WIDGET)
    {
        return -1;
    }
    Image *i = malloc(sizeof(Image));
    i->image = image;
    Widget *widget = &win->widgets[win->widget_number];
    widget->paint = drawImageWidget;
    widget->type = IMAGE;
    widget->context.image = i;
    setWidgetSize(widget, x, y, w, h);
    win->widget_number++;
    return (win->widget_number - 1);
}

int addLabelWidget(window *win, RGBA c, char *text, int x, int y, int w, int h)
{
    if (win->widget_number >= MAX_WIDGET)
    {
        return -1;
    }
    Label *l = malloc(sizeof(Label));
    l->color = c;
    strcpy(l->text, text);
    Widget *widget = &win->widgets[win->widget_number];
    widget->paint = drawLabelWidget;
    widget->type = LABEL;
    widget->context.label = l;
    setWidgetSize(widget, x, y, w, h);
    win->widget_number++;
    return (win->widget_number - 1);
}

int addButtonWidget(window *win, RGBA c, RGBA bc, char *text, Handler handler, int x, int y, int w, int h)
{
    if (win->widget_number >= MAX_WIDGET)
    {
        return -1;
    }
    Button *b = malloc(sizeof(Button));
    b->bg_color = bc;
    b->color = c;
    strcpy(b->text, text);
    b->onLeftClick = handler;
    Widget *widget = &win->widgets[win->widget_number];
    widget->paint = drawButtonWidget;
    widget->context.button = b;
    widget->type = BUTTON;
    setWidgetSize(widget, x, y, w, h);
    win->widget_number++;
    return (win->widget_number - 1);
}

int addInputWidget(window *win, RGBA c, char *text, int x, int y, int w, int h)
{
    if (win->widget_number >= MAX_WIDGET)
    {
        return -1;
    }
    Input *i = malloc(sizeof(Input));
    i->color = c;
    i->current_pos = 0;
    strcpy(i->text, text);
    // TODO: add default handler
    Widget *widget = &win->widgets[win->widget_number];
    widget->paint = drawInputWidget;
    widget->context.input = i;
    widget->type = INPUT;
    setWidgetSize(widget, x, y, w, h);
    win->widget_number++;
    return (win->widget_number - 1);
}

int addTextAreaWidget(window *win, RGBA c, char *text, int x, int y, int w, int h)
{
    if (win->widget_number >= MAX_WIDGET)
    {
        return -1;
    }
    TextArea *t = malloc(sizeof(TextArea));
    t->color = c;
    for(int i=0;i<MAX_LONG_STRLEN;i++){
        t->colors[i] =c;
    }
    t->current_pos = 0;
    t->current_line = 0;
    t->select_start_index = -2;
    t->select_end_index = -2;
    t->isCopying = 0;
    t->begin_line = 0;
    t->temp = 0;
    strcpy(t->text, text);
    t->onKeyDown = textAreaKeyDownHandler;
    Widget *widget = &win->widgets[win->widget_number];
    widget->paint = drawTextAreaWidget;
    widget->context.textArea = t;
    widget->type = TEXT_AREA;
    setWidgetSize(widget, x, y, w, h);
    win->widget_number++;
    return (win->widget_number - 1);
}

char *UI_fmtname(char *path)
{
    char *p;
    // Find first character after last slash.
    for (p = path + strlen(path); p >= path && *p != '/'; p--)
        ;
    p++;
    return p;
}

void UI_ls(char *path, Widget *widget)
{
    char buf[512], *p, *tmpName;
    int fd;
    struct dirent de;
    struct stat st;
    IconView *node = 0;
    int j;
    if ((fd = open(path, 0)) < 0)
    {
        return;
    }
    if (fstat(fd, &st) < 0)
    {
        return;
    }
    strcpy(buf, path);
    p = buf + strlen(buf);
    *p++ = '/';
    while (read(fd, &de, sizeof(de)) == sizeof(de))
    {
        if (de.inum == 0)
        {
            continue;
        }
        memmove(p, de.name, DIRSIZ);
        p[DIRSIZ] = 0;
        if (stat(buf, &st) < 0)
        {
            continue;
        }
        tmpName = UI_fmtname(buf);
        if (strcmp(tmpName, ".") == 0 || strcmp(tmpName, "..") == 0 || strcmp(tmpName, "desktop") == 0 || st.type == T_DEV || strcmp(tmpName, "desktop.bmp") == 0 || strcmp(tmpName, "init") == 0)
        {
            continue;
        }
        IconView *iconView = malloc(sizeof(IconView));
        iconView->next = 0;
        for (j = 0; j < strlen(tmpName); j++)
        {
            if (tmpName[j] == '.')
                break;
        }
        switch (st.type)
        {
        case T_DIR:
            iconView->image = widget->context.fileList->image[FOLDER_FILE];
            break;
        case T_FILE:
            if (strcmp(tmpName, "README") == 0)
                iconView->image = widget->context.fileList->image[UNKNOWN_FILE];
            else if (tmpName[j + 1] == 'b' && tmpName[j + 2] == 'm' && tmpName[j + 3] == 'p')
                iconView->image = widget->context.fileList->image[BMP_FILE];
            else if (tmpName[j + 1] == 't' && tmpName[j + 2] == 'x' && tmpName[j + 3] == 't')
                iconView->image = widget->context.fileList->image[TEXT_FILE];
            else
                iconView->image = widget->context.fileList->image[EXEC_FILE];
            break;
        }
        strcpy(iconView->text, tmpName);
        if (!widget->context.fileList->file_list)
        {
            widget->context.fileList->file_list = iconView;
        }
        if (!node)
        {
            node = iconView;
        }
        else
        {
            node->next = iconView;
            node = iconView;
        }
        widget->context.fileList->file_num += 1;
    }
}

int addFileListWidget(window *win, char *path, int direction, int x, int y, int w, int h)
{
    if (win->widget_number >= MAX_WIDGET)
    {
        return -1;
    }
    FileList *f = malloc(sizeof(FileList));
    f->direction = direction;
    f->file_num = 0;
    strcpy(f->path, path);
    int i;
    int res;
    int temp_w, temp_h;
    for (i = 0; i < FILE_TYPE_NUM; i++)
    { // read file
        (f->image[i]) = (RGBA *)malloc(ICON_IMG_SIZE * ICON_IMG_SIZE * 4);
        res = readBitmapFile(file_image_path[i], f->image[i], &temp_h, &temp_w);
        if (res < 0)
        {
            printf(1, "read file image error \n");
            for (i--; i >= 0; i--)
            {
                free(f->image[i]);
            }
            return -1;
        }
    }
    // TODO: set default handler
    f->onDoubleClick = fileListDoubleClickHandler;
    Widget *widget = &win->widgets[win->widget_number];
    widget->paint = drawFileListWidget;
    widget->context.fileList = f;
    widget->type = FILE_LIST;
    setWidgetSize(widget, x, y, w, h);
    UI_ls(path, widget);
    win->widget_number++;
    return (win->widget_number - 1);
}

// paint function

void drawPointAlpha(RGB *color, RGBA origin)
{
    float alpha;
    if (origin.A == 255)
    {
        color->R = origin.R;
        color->G = origin.G;
        color->B = origin.B;
        return;
    }
    if (origin.A == 0)
    {
        return;
    }
    alpha = (float)origin.A / 255;
    color->R = color->R * (1 - alpha) + origin.R * alpha;
    color->G = color->G * (1 - alpha) + origin.G * alpha;
    color->B = color->B * (1 - alpha) + origin.B * alpha;
}

int drawCharacterWithBg(window *win, int x, int y, char ch, RGBA color, RGBA bg)
{
    int i, j;
    RGB *t;
    int ord = ch - 0x20;
    if (ord < 0 || ord >= (CHARACTER_NUMBER))
    {
        return -1;
    }
    for (i = 0; i < CHARACTER_HEIGHT; i++)
    {
        if (y + i > win->height)
        {
            break;
        }
        if (y + i < 0)
        {
            continue;
        }
        for (j = 0; j < CHARACTER_WIDTH; j++)
        {

            if (x + j > win->width)
            {
                break;
            }
            if (x + j < 0)
            {
                continue;
            }
            t = win->window_buf + (y + i) * win->width + x + j;
            if (character[ord][i][j] == 1)
            {
                drawPointAlpha(t, color);
            }
            else
            {
                drawPointAlpha(t, bg);
            }
        }
    }
    return CHARACTER_WIDTH;
}

int drawCharacter(window *win, int x, int y, char ch, RGBA color)
{
    int i, j;
    RGB *t;
    int ord = ch - 0x20;
    if (ord < 0 || ord >= (CHARACTER_NUMBER))
    {
        return -1;
    }
    for (i = 0; i < CHARACTER_HEIGHT; i++)
    {
        if (y + i > win->height)
        {
            break;
        }
        if (y + i < 0)
        {
            continue;
        }
        for (j = 0; j < CHARACTER_WIDTH; j++)
        {
            if (character[ord][i][j] == 1)
            {
                if (x + j > win->width)
                {
                    break;
                }
                if (x + j < 0)
                {
                    continue;
                }
                t = win->window_buf + (y + i) * win->width + x + j;
                drawPointAlpha(t, color);
            }
        }
    }
    return CHARACTER_WIDTH;
}

void drawString(window *win, int x, int y, char *str, RGBA color, int width)
{
    int offset_x = 0;

    while (*str != '\0')
    {
        if (x + offset_x >= win->width || offset_x >= width)
        { // if too long
            break;
        }
        offset_x += drawCharacter(win, x + offset_x, y, *str, color);
        str++;
    }
}

void drawImage(window *win, RGBA *img, int x, int y, int width, int height)
{
    int i, j;
    RGB *t;
    RGBA *o;
    for (i = 0; i < height; i++)
    {
        if (y + i >= win->height)
        {
            break;
        }
        if (y + i < 0)
        {
            continue;
        }
        for (j = 0; j < width; j++)
        {
            if (x + j >= win->width)
            {
                break;
            }
            if (x + j < 0)
            {
                continue;
            }
            t = win->window_buf + (y + i) * win->width + x + j;
            o = img + (height - i - 1) * width + j;
            drawPointAlpha(t, *o);
        }
    }
}

void draw24Image(window *win, RGB *img, int x, int y, int width, int height)
{
    int i;
    RGB *t;
    RGB *o;
    int max_line = (win->width - x) < width ? (win->width - x) : width;
    for (i = 0; i < height; i++)
    {
        if (y + i >= win->height)
        {
            break;
        }
        if (y + i < 0)
        {
            continue;
        }
        t = win->window_buf + (y + i) * win->width + x;
        o = img + (height - i - 1) * width;
        memmove(t, o, max_line * 3);
    }
}

void drawRect(window *win, RGB color, int x, int y, int width, int height)
{
    if (x >= win->width || x + width < 0 || y >= win->height || y + height < 0 || x < 0 || y < 0 || width < 0 || height < 0)
    {
        return;
    }
    int i;
    int max_line = (win->width - x) < width ? (win->width - x) : width;
    RGB *t = win->window_buf + y * win->width + x;
    for (i = 0; i < max_line; i++)
    {
        *(t + i) = color;
    }
    if (y + height < win->height)
    {
        RGB *o = win->window_buf + (y + height) * win->width + x;
        memmove(o, t, max_line * 3);
    }
    int max_height = (win->height - y) < height ? (win->height - x) : height;
    for (i = 0; i < max_height; i++)
    {
        *(t + i * win->width) = color;
    }
    if (x + width < win->width)
    {
        t = win->window_buf + y * win->width + x + win->width;
        for (i = 0; i < max_height; i++)
        {
            *(t + i * win->width) = color;
        }
    }
}

void drawFillRect(window *win, RGBA color, int x, int y, int width, int height)
{
    if (x >= win->width || x + width < 0 || y >= win->height || y + height < 0 || x < 0 || y < 0 || width < 0 || height < 0)
    {
        return;
    }
    int i, j;
    RGB *t;
    for (i = 0; i < height; i++)
    {
        if (y + i >= win->height)
        {
            break;
        }
        if (y + i < 0)
        {
            continue;
        }
        for (j = 0; j < width; j++)
        {
            if (j + x >= win->width)
            {
                break;
            }
            if (j + x < 0)
            {
                continue;
            }
            t = win->window_buf + (y + i) * win->width + (x + j);
            drawPointAlpha(t, color);
        }
    }
}

void draw24FillRect(window *win, RGB color, int x, int y, int width, int height)
{
    if (x >= win->width || x + width < 0 || y >= win->height || y + height < 0 || x < 0 || y < 0 || width < 0 || height < 0)
    {
        return;
    }
    int i, j;
    int max_line = (win->width - x) < width ? (win->width - x) : width;
    RGB *t, *o;
    t = win->window_buf + y * win->width + x;
    for (i = 0; i < height; i++)
    {
        if (y + i >= win->height)
        {
            break;
        }
        if (y + i < 0)
        {
            continue;
        }
        if (i == 0)
        {
            for (j = 0; j < max_line; j++)
            {
                *(t + j) = color;
            }
        }
        else
        {
            o = win->window_buf + (y + i) * win->width + x;
            memmove(o, t, max_line * 3);
        }
    }
}

void drawImageWidget(window *win, int index)
{
    Widget *w = &(win->widgets[index]);
    draw24Image(win, w->context.image->image, w->size.x, w->size.y, w->size.width, w->size.height);
}

void drawLabelWidget(window *win, int index)
{
    Widget *w = &(win->widgets[index]);
    drawString(win, w->size.x, w->size.y, w->context.label->text, w->context.label->color, w->size.width);
}

void drawButtonWidget(window *win, int index)
{
    Widget *w = &(win->widgets[index]);
    RGB black;
    black.R = 0;
    black.G = 0;
    black.B = 0;
    drawFillRect(win, w->context.button->bg_color, w->size.x, w->size.y, w->size.width, w->size.height);
    drawRect(win, black, w->size.x, w->size.y, w->size.width, w->size.height);
    drawString(win, w->size.x, w->size.y, w->context.button->text, w->context.button->color, w->size.width);
}

void drawInputWidget(window *win, int index)
{
    Widget *w = &(win->widgets[index]);
    RGB black, white;
    black.R = 0;
    black.G = 0;
    black.B = 0;
    white.R = 255;
    white.G = 255;
    white.B = 255;
    draw24FillRect(win, white, w->size.x, w->size.y, w->size.width, w->size.height);
    drawRect(win, black, w->size.x, w->size.y, w->size.width, w->size.height);
    drawString(win, w->size.x + 3, w->size.y + 2, w->context.button->text, w->context.input->color, w->size.width - 3);
}

void drawTextAreaWidget(window *win, int index)
{
    Widget *w = &(win->widgets[index]);

    RGB white;
    white.R = 255;
    white.G = 255;
    white.B = 255;
    draw24FillRect(win, white, w->size.x, w->size.y, w->size.width, w->size.height);

    // for select
    RGBA gray;
    gray.A = 255;
    gray.R = 214;
    gray.G = 214;
    gray.B = 194;

    int max_num = w->size.width / CHARACTER_WIDTH;
    int max_line = w->size.height / CHARACTER_HEIGHT;

    int current_x = 0;
    int current_y = 0;

    int i;

    generateHighlightRGB(w);

    for (i = 0; w->context.textArea->text[i] || (current_x == w->context.textArea->current_pos && current_y == w->context.textArea->current_line); i++)
    {
        char ch = w->context.textArea->text[i];
        int newline = 0;
        if (current_x == w->context.textArea->current_pos && current_y == w->context.textArea->current_line)
        {
            ch = 95 + 0x20;
            w->context.textArea->insert_index = i;
            i--;
        }
        else if (w->context.textArea->text[i] == '\n')
        {
            newline=1;
            current_y++;
            current_x = 0;
            if (current_y >= max_line)
            {
                break;
            }
        }

        if (i >= w->context.textArea->select_start_index && i <= w->context.textArea->select_end_index && ch != (95 + 0x20))
        {
            drawCharacterWithBg(win, w->size.x + current_x * CHARACTER_WIDTH, w->size.y + current_y * CHARACTER_HEIGHT,
                                ch, w->context.textArea->colors[i], gray);
        }
        else
        {
            drawCharacter(win, w->size.x + current_x * CHARACTER_WIDTH, w->size.y + current_y * CHARACTER_HEIGHT,
                          ch, w->context.textArea->colors[i]);
        }
        if(!newline){
            current_x++;
        }
        if (current_x >= max_num)
        {
            current_x = 0;
            current_y++;
            if (current_y >= max_line)
            {
                break;
            }
        }
    }
}

void drawFileListWidget(window *win, int index)
{
    Widget *w = &(win->widgets[index]);

    int max_num_y = w->size.height / ICON_VIEW_SIZE;
    int max_num_x = w->size.width / ICON_VIEW_SIZE;
    int offset_y = w->size.height % ICON_VIEW_SIZE / max_num_y;
    int offset_x = w->size.width % ICON_VIEW_SIZE / max_num_x;
    int current_x = 0;
    int current_y = 0;

    IconView *p = w->context.fileList->file_list;
    int i;
    RGBA white;
    white.A = 255;
    white.R = 180;
    white.G = 180;
    white.B = 120;
    for (i = 0; i < w->context.fileList->file_num; i++)
    {
        drawImage(win, p->image, offset_x + current_x * ICON_VIEW_SIZE + 13,
                  offset_y + current_y * ICON_VIEW_SIZE + 4, ICON_IMG_SIZE, ICON_IMG_SIZE);
        if (strlen(p->text) <= 8)
        {
            drawString(win, offset_x + current_x * ICON_VIEW_SIZE + (8 - strlen(p->text)) * 8 / 2 + 5,
                       offset_y + current_y * ICON_VIEW_SIZE + 4 + ICON_IMG_SIZE, p->text, white,
                       ICON_VIEW_SIZE - 2);
        }
        else
        {
            char temp[9];
            int j;
            for (j = 0; j < 8; j++)
            {
                temp[j] = p->text[j];
            }
            temp[8] = '\0';
            drawString(win, offset_x + current_x * ICON_VIEW_SIZE + 5,
                       offset_y + current_y * ICON_VIEW_SIZE + ICON_IMG_SIZE, temp,
                       white, ICON_VIEW_SIZE - 2);
            for (j = 0; j < 9; j++)
            {
                if (!p->text[j + 8])
                {
                    break;
                }
                temp[j] = p->text[j + 8];
            }
            temp[j] = '\0';
            drawString(win, offset_x + current_x * ICON_VIEW_SIZE + (8 - strlen(temp)) * 8 / 2 + 5,
                       offset_y + current_y * ICON_VIEW_SIZE + ICON_IMG_SIZE + 16, temp,
                       white, ICON_VIEW_SIZE - 2);
        }
        if (w->context.fileList->direction == 0)
        {
            current_y++;
            p = p->next;
            if (current_y >= max_num_y)
            {
                current_y = 0;
                current_x++;
                if (current_x >= max_num_x)
                {
                    break;
                }
            }
        }
        else
        {
            current_x++;
            p = p->next;
            if (current_x >= max_num_x)
            {
                current_x = 0;
                current_y++;
                if (current_y >= max_num_y)
                {
                    break;
                }
            }
        }
    }
}

void drawAllWidget(window *win)
{
    int i;
    for (i = 0; i < win->widget_number; i++)
    {
        win->widgets[i].paint(win, i);
    }
    updatewindow(win->handler, 0, 0, win->width, win->height);
}

void UI_suffix(char *t, char *s)
{
    int point = 0;

    while (*s != 0)
    {
        if (*s == '.')
            point = 1;
        s++;
    }
    if (point == 0)
    {
        strcpy(t, "");
        return;
    }
    while (*s != '.')
        s--;
    s++;
    strcpy(t, s);
}

int min(int a, int b)
{
    return a < b ? a : b;
}

int max(int a, int b)
{
    return a > b ? a : b;
}

void generateHighlightRGB(Widget * w)
{
    char* text = w->context.textArea->text;
    int len = strlen(text);
    RGBA* colors = w->context.textArea->colors;

    struct RGBA red = (struct RGBA){180,20,20,255};
    
    
    for (int i = 0; i <  len && text[i]; i++)
    {
        // printf(1, "\e[1;30m%d%d%d\e[0;32m| \e[0m", (i + 1) / 100, (i + 1) % 100 / 10, (i + 1) % 10);
        // find next \n
        if(text[i]=='\n'){
            continue;
        }

        int newlineIndex=len;
        for(int j=i;j<len;j++){
            if(text[j]=='\n'){
                newlineIndex=j;
                break;
            }
        }

        // annotation
        if (text[i] == '/' && text[i+1] == '/')
        {
            for(int j=i;j<newlineIndex;j++){
                colors[j]= (struct RGBA){120,120,120,255};
                i++;
            }
            continue;
        }

        while (i<newlineIndex)
        {
            // numbers
            if (text[i] >= '0' && text[i] <= '9')
            {
                colors[i]=(struct RGBA){60,120,120,255};
                i+=1;
            }

            // string
            // TODO: fix possible segement fault
            else if (text[i] == '"')
            {
                int start = i;
                int end = i+1;
                for (; text[end] != '"' && text[end] != '\0'; end++)
                    ;
                for (int i = start; i <= end; i++)
                {
                    colors[i] = (struct RGBA){10,120,120,255};
                }
                i = end;
            }

            // single char
            else if (text[i] == '\'')
            {
                if (i + 1 < len)
                    colors[i+1] = (struct RGBA){10,10,120,255};
                if (i + 2 < len)
                    colors[i+2] = (struct RGBA){10,10,120,255};
                i += 3;
            }

            // int
            else if ((len - i) >= 3 && text[i] == 'i' && text[i+1] == 'n' && text[i+2]== 't')
            {
                colors[i] = (struct RGBA){10,10,120,255};
                colors[i+1] = (struct RGBA){10,10,120,255};
                colors[i+2] = (struct RGBA){10,10,120,255};
                i += 3;
            }

            // long
            else if ((len - i) >= 4 && text[i] == 'l' && text[i+1]== 'o' && text[i+2]== 'n' && text[i+3]== 'g')
            {
                colors[i] = (struct RGBA){10,10,10,255};
                colors[i+1] = (struct RGBA){10,10,10,255};
                colors[i+2] = (struct RGBA){10,10,10,255};
                colors[i+3] = (struct RGBA){10,10,10,255};
                i += 4;
            }

            // double
            else if ((len - i) >= 6 && text[i] == 'd' && text[i+1]== 'o' && text[i+2]== 'u' && text[i+3] == 'b' && text[i+4]== 'l' && text[i+ 5] == 'e')
            {
                colors[i] = (struct RGBA){10,10,10,255};
                colors[i+1] = (struct RGBA){10,10,10,255};
                colors[i+2] = (struct RGBA){10,10,10,255};
                colors[i+3] = (struct RGBA){10,10,10,255};
                colors[i+4] = (struct RGBA){10,10,10,255};
                colors[i+5] = (struct RGBA){10,10,10,255};
                i += 6;
            }

            // float
            else if ((len - i) >= 5 && text[i] == 'f' && text[i + 1] == 'l' && text[i + 2] == 'o' && text[i + 3] == 'a' && text[i + 4] == 't')
            {
                colors[i] = (struct RGBA){10,10,10,255};
                colors[i+1] = (struct RGBA){10,10,10,255};
                colors[i+2] = (struct RGBA){10,10,10,255};
                colors[i+3] = (struct RGBA){10,10,10,255};
                colors[i+4] = (struct RGBA){10,10,10,255};
                i += 5;
            }

            // char
            else if ((len - i) >= 4 && text[i] == 'c' && text[i + 1] == 'h' && text[i + 2] == 'a' && text[i + 3] == 'r')
            {
                colors[i] = (struct RGBA){10,10,10,255};
                colors[i+1] = (struct RGBA){10,10,10,255};
                colors[i+2] = (struct RGBA){10,10,10,255};
                colors[i+3] = (struct RGBA){10,10,10,255};
                i += 4;
            }

            // if
            else if ((len - i) >= 2 && text[i] == 'i' && text[i + 1] == 'f')
            {
                colors[i] = (struct RGBA){10,10,10,255};
                colors[i+1] = (struct RGBA){10,10,10,255};
                i += 2;
            }

            // else
            else if ((len - i) >= 4 && text[i] == 'e' && text[i + 1] == 'l' && text[i + 2] == 's' && text[i + 3] == 'e')
            {
                colors[i] = (struct RGBA){10,10,10,255};
                colors[i+1] = (struct RGBA){10,10,10,255};
                colors[i+2] = (struct RGBA){10,10,10,255};
                colors[i+3] = (struct RGBA){10,10,10,255};
                i += 4;
            }

            // else if
            else if ((len - i) >= 7 && text[i] == 'e' && text[i + 1] == 'l' && text[i + 2] == 's' && text[i + 3] == 'e' && text[i + 4] == ' ' && text[i + 5] == 'i' && text[i + 6] == 'f')
            {
                colors[i] = (struct RGBA){10,10,10,255};
                colors[i+1] = (struct RGBA){10,10,10,255};
                colors[i+2] = (struct RGBA){10,10,10,255};
                colors[i+3] = (struct RGBA){10,10,10,255};
                colors[i+4] = (struct RGBA){10,10,10,255};
                colors[i+5] = (struct RGBA){10,10,10,255};
                colors[i+6] = (struct RGBA){10,10,10,255};
                i += 7;
            }

            // for
            else if ((len - i) >= 3 && text[i] == 'f' && text[i + 1] == 'o' && text[i + 2] == 'r')
            {
                colors[i] = (struct RGBA){10,10,10,255};
                colors[i+1] = (struct RGBA){10,10,10,255};
                colors[i+2] = (struct RGBA){10,10,10,255};
                i += 3;
            }

            // while
            else if ((len - i) >= 5 && text[i] == 'w' && text[i + 1] == 'h' && text[i + 2] == 'i' && text[i + 3] == 'l' && text[i + 4] == 'e')
            {
                colors[i] = (struct RGBA){10,10,10,255};
                colors[i+1] = (struct RGBA){10,10,10,255};
                colors[i+2] = (struct RGBA){10,10,10,255};
                colors[i+3] = (struct RGBA){10,10,10,255};
                colors[i+4] = (struct RGBA){10,10,10,255};
                i += 5;
            }

            // static
            else if ((len - i) >= 6 && text[i] == 's' && text[i + 1] == 't' && text[i + 2] == 'a' && text[i + 3] == 't' && text[i + 4] == 'i' && text[i + 5] == 'c')
            {
                colors[i] = (struct RGBA){10,10,10,255};
                colors[i+1] = (struct RGBA){10,10,10,255};
                colors[i+2] = (struct RGBA){10,10,10,255};
                colors[i+3] = (struct RGBA){10,10,10,255};
                colors[i+4] = (struct RGBA){10,10,10,255};
                i += 6;
            }

            // const
            else if ((len - i) >= 5 && text[i] == 'c' && text[i + 1] == 'o' && text[i + 2] == 'n' && text[i + 3] == 's' && text[i + 4] == 't')
            {
                colors[i] = (struct RGBA){10,10,10,255};
                colors[i+1] = (struct RGBA){10,10,10,255};
                colors[i+2] = (struct RGBA){10,10,10,255};
                colors[i+3] = (struct RGBA){10,10,10,255};
                colors[i+4] = (struct RGBA){10,10,10,255};
                i +=5;
            }

            else if ((len - i) >= 8 && text[i] == 'c' && text[i + 1] == 'o' && text[i + 2] == 'n' && text[i + 3] == 't' && text[i + 4] == 'i' && text[i + 5] == 'n' && text[i + 6] == 'u' && text[i + 7] == 'e')
            {
                colors[i] = (struct RGBA){10,10,10,255};
                colors[i+1] = (struct RGBA){10,10,10,255};
                colors[i+2] = (struct RGBA){10,10,10,255};
                colors[i+3] = (struct RGBA){10,10,10,255};
                colors[i+4] = (struct RGBA){10,10,10,255};
                colors[i+5] = (struct RGBA){10,10,10,255};
                colors[i+6] = (struct RGBA){10,10,10,255};
                colors[i+7] = (struct RGBA){10,10,10,255};
                i += 8;
            }

            else if ((len - i) >= 6 && text[i] == 'r' && text[i + 1]== 'e' && text[i + 2] == 't' && text[i + 3] == 'u' && text[i + 4] == 'r' && text[i + 5] == 'n')
            {
                colors[i] = (struct RGBA){10,10,10,255};
                colors[i+1] = (struct RGBA){10,10,10,255};
                colors[i+2] = (struct RGBA){10,10,10,255};
                colors[i+3] = (struct RGBA){10,10,10,255};
                colors[i+4] = (struct RGBA){10,10,10,255};
                colors[i+5] = (struct RGBA){10,10,10,255};
                i += 6;
            }

            else if ((len - i) >= 5 && text[i] == 'b' && text[i + 1] == 'r' && text[i + 2] == 'e' && text[i + 3] == 'a' && text[i + 4] == 'k')
            {
                colors[i] = (struct RGBA){10,10,10,255};
                colors[i+1] = (struct RGBA){10,10,10,255};
                colors[i+2] = (struct RGBA){10,10,10,255};
                colors[i+3] = (struct RGBA){10,10,10,255};
                colors[i+4] = (struct RGBA){10,10,10,255};
                i += 5;
            }

            else
            {
                i += 1;
            }
        }
    }
}

// Attention
// msg->params[1] & 2 == 2 Ctrl
// msg->params[1] & 1 == 1 Shift

void textAreaKeyDownHandler(window *win, int index, message *msg)
{
    Widget *w = &(win->widgets[index]);

    int max_num = w->size.width / CHARACTER_WIDTH;
    // int max_line = w->size.height / CHARACTER_HEIGHT;
    int current_pos = w->context.textArea->current_pos;
    int current_line = w->context.textArea->current_line;

    int len = strlen(w->context.textArea->text);

    if (msg->msg_type == M_KEY_DOWN)
    {

        // printf(1, "DEBUG %x\n", msg->params[0]);

        if (msg->params[0] == '\b')
        {
            if(len<1){
                return;
            }
            for(int i=w->context.textArea->insert_index-1;i<len-1;i++){
                w->context.textArea->text[i]=w->context.textArea->text[i+1];
            }
            w->context.textArea->text[len - 1] = '\0';

            if (current_pos >= 1)
            {
                w->context.textArea->current_pos--;
            }
            else
            {
                if (current_line >= 1)
                {

                    // w->context.textArea->current_line--;
                    // w->context.textArea->current_pos = max_num - 1;
                    int current_line=0;
                    int current_pos = 0;
                    for(int i=0;i<w->context.textArea->insert_index-1;i++){
                        if(w->context.textArea->text[i]=='\n'){
                            current_line++;
                            current_pos=0;
                        }
                        else{
                            current_pos++;
                        }
                    }
                    w->context.textArea->current_line=current_line;
                    w->context.textArea->current_pos = current_pos;

                }
            }

            drawTextAreaWidget(win, index);
            updatePartWindow(win, w->size.x, w->size.y, w->size.width, w->size.height);
            return;
        }

        if (len >= MAX_LONG_STRLEN - 1)
        {
            return;
        }

        // Ctrl
        if ((msg->params[1] & 2) == 2)
        {
            if (msg->params[0] == 'c')
            {
                w->context.textArea->isCopying = 1;
                w->context.textArea->copy_start_index = w->context.textArea->select_start_index;
                w->context.textArea->copy_end_index = w->context.textArea->select_end_index;
                int copyTextLength = w->context.textArea->copy_end_index - w->context.textArea->copy_start_index + 1;
                if (w->context.textArea->temp != 0)
                    free(w->context.textArea->temp);
                w->context.textArea->temp = (char *)malloc(copyTextLength);
                for (int i = 0; i < copyTextLength; i++)
                {
                    w->context.textArea->temp[i] = w->context.textArea->text[w->context.textArea->copy_start_index + i];
                }
            }
            if (msg->params[0] == 'v')
            {
                if (w->context.textArea->isCopying == 0)
                {
                    return;
                }
                printf(1, "START: %d", w->context.textArea->copy_start_index);
                int copyTextLength = w->context.textArea->copy_end_index - w->context.textArea->copy_start_index + 1;
                for (int i = len + copyTextLength - 1; i >= current_line * max_num + current_pos + copyTextLength; i--)
                {
                    w->context.textArea->text[i] = w->context.textArea->text[i - copyTextLength];
                }
                w->context.textArea->text[len + copyTextLength] = '\0';
                for (int i = current_line * max_num + current_pos; i < current_line * max_num + current_pos + copyTextLength; i++)
                {
                    int j = i - current_line * max_num - current_pos;
                    w->context.textArea->text[i] = w->context.textArea->temp[j];
                    printf(1, "%d:%c\n", i, w->context.textArea->temp[j]);
                }

                // w->context.textArea->copy_start_index = w->context.textArea->copy_end_index = -2;
                w->context.textArea->current_line = (current_line * max_num + current_pos + copyTextLength) / max_num;
                w->context.textArea->current_pos = (current_line * max_num + current_pos + copyTextLength) % max_num;
                // w->context.textArea->isCopying =0;
                drawTextAreaWidget(win, index);
                updatePartWindow(win, w->size.x, w->size.y, w->size.width, w->size.height);
                return;
            }
            if (msg->params[0] == 'a')
            {
                w->context.textArea->isCopying = 1;
                w->context.textArea->select_start_index = 0;
                w->context.textArea->select_end_index = len - 1;
                drawTextAreaWidget(win, index);
                updatePartWindow(win, w->size.x, w->size.y, w->size.width, w->size.height);
                return;
            }
            if (msg->params[0] == 'x')
            {
                w->context.textArea->isCopying = 1;
                w->context.textArea->copy_start_index = w->context.textArea->select_start_index;
                w->context.textArea->copy_end_index = w->context.textArea->select_end_index;
                int copyTextLength = w->context.textArea->copy_end_index - w->context.textArea->copy_start_index + 1;
                if (w->context.textArea->temp != 0)
                    free(w->context.textArea->temp);
                w->context.textArea->temp = (char *)malloc(copyTextLength);
                for (int i = 0; i < copyTextLength; i++)
                {
                    w->context.textArea->temp[i] = w->context.textArea->text[w->context.textArea->copy_start_index + i];
                }
                for (int i = w->context.textArea->copy_start_index; i < len - copyTextLength; i++)
                {
                    w->context.textArea->text[i] = w->context.textArea->text[i + copyTextLength];
                }
                w->context.textArea->text[len - copyTextLength] = '\0';
                drawTextAreaWidget(win, index);
                updatePartWindow(win, w->size.x, w->size.y, w->size.width, w->size.height);
                return;
            }

            if (msg->params[0] == KEY_HOME)
            {
                w->context.textArea->current_line = 0;
                w->context.textArea->current_pos = 0;
                drawTextAreaWidget(win, index);
                updatePartWindow(win, w->size.x, w->size.y, w->size.width, w->size.height);
                return;
            }

            if (msg->params[0] == KEY_END)
            {
                w->context.textArea->current_line = len / max_num;
                w->context.textArea->current_pos = len % max_num;
                drawTextAreaWidget(win, index);
                updatePartWindow(win, w->size.x, w->size.y, w->size.width, w->size.height);
                return;
            }

            return;
        }

        // Home End
        if (msg->params[0] == KEY_HOME)
        {
            if ((msg->params[1] & 1) == 1)
            {
                w->context.textArea->select_start_index = current_line * max_num;
                w->context.textArea->select_end_index = current_line * max_num + current_pos;
            }
            else
            {
                w->context.textArea->select_start_index = -2;
                w->context.textArea->select_end_index = -2;
            }
            w->context.textArea->current_pos = 0;
            drawTextAreaWidget(win, index);
            updatePartWindow(win, w->size.x, w->size.y, w->size.width, w->size.height);
            return;
        }

        if (msg->params[0] == KEY_END)
        {
            if ((msg->params[1] & 1) == 1)
            {
                w->context.textArea->select_start_index = current_line * max_num + current_pos;
                w->context.textArea->select_end_index = current_line * max_num + max_num - 1;
            }
            else
            {
                w->context.textArea->select_start_index = -2;
                w->context.textArea->select_end_index = -2;
            }
            w->context.textArea->current_pos = len % max_num;
            drawTextAreaWidget(win, index);
            updatePartWindow(win, w->size.x, w->size.y, w->size.width, w->size.height);
            return;
        }

        // Up Down Left Right
        if (msg->params[0] == KEY_LF)
        {
            if (current_pos >= 1)
            {
                w->context.textArea->current_pos--;
            }
            else
            {
                if (current_line >= 1)
                {
                    w->context.textArea->current_line--;
                    w->context.textArea->current_pos = max_num - 1;
                }
            }
            if ((msg->params[1] & 1) == 1)
            {
                if (w->context.textArea->select_start_index < 0)
                {
                    w->context.textArea->select_start_index = current_line * max_num + current_pos - 1;
                    w->context.textArea->select_end_index = w->context.textArea->select_start_index;
                }
                else
                {
                    if (w->context.textArea->select_start_index == current_line * max_num + current_pos)
                    {
                        w->context.textArea->select_start_index = w->context.textArea->current_line * max_num + w->context.textArea->current_pos;
                    }
                    else
                    {
                        w->context.textArea->select_end_index = w->context.textArea->current_line * max_num + w->context.textArea->current_pos - 1;
                    }
                }
            }
            else
            {
                w->context.textArea->select_start_index = -2;
                w->context.textArea->select_end_index = -2;
            }
            drawTextAreaWidget(win, index);
            updatePartWindow(win, w->size.x, w->size.y, w->size.width, w->size.height);
            return;
        }

        if (msg->params[0] == KEY_RT)
        {
            if ((current_line * max_num + current_pos) < len)
            {
                if (current_pos == max_num - 1)
                {
                    w->context.textArea->current_line++;
                    w->context.textArea->current_pos = 0;
                }
                else
                {
                    w->context.textArea->current_pos++;
                }
            }
            if ((msg->params[1] & 1) == 1)
            {
                if (w->context.textArea->select_start_index < 0)
                {
                    w->context.textArea->select_start_index = current_line * max_num + current_pos;
                    w->context.textArea->select_end_index = w->context.textArea->select_start_index;
                }
                else
                {
                    if (w->context.textArea->select_start_index == current_line * max_num + current_pos)
                    {
                        w->context.textArea->select_start_index = w->context.textArea->current_line * max_num + w->context.textArea->current_pos;
                    }
                    else
                    {
                        w->context.textArea->select_end_index = w->context.textArea->current_line * max_num + w->context.textArea->current_pos - 1;
                    }
                }
            }
            else
            {
                w->context.textArea->select_start_index = -2;
                w->context.textArea->select_end_index = -2;
            }

            drawTextAreaWidget(win, index);
            updatePartWindow(win, w->size.x, w->size.y, w->size.width, w->size.height);
            return;
        }

        if (msg->params[0] == KEY_UP)
        {
            if ((current_line * max_num + current_pos) >= max_num)
            {
                w->context.textArea->current_line--;
            }
            if ((msg->params[1] & 1) == 1)
            {
                if (w->context.textArea->select_end_index < 0)
                {
                    w->context.textArea->select_end_index = current_line * max_num + current_pos - 1;
                }

                w->context.textArea->select_start_index = w->context.textArea->current_line * max_num + current_pos;
            }
            else
            {
                w->context.textArea->select_start_index = -2;
                w->context.textArea->select_end_index = -2;
            }
            drawTextAreaWidget(win, index);
            updatePartWindow(win, w->size.x, w->size.y, w->size.width, w->size.height);
            return;
        }
        if (msg->params[0] == KEY_DN)
        {
            if (((current_line + 1) * max_num + current_pos) <= len)
            {
                w->context.textArea->current_line++;
            }
            if ((msg->params[1] & 1) == 1)
            {
                if (w->context.textArea->select_end_index < 0)
                {
                    w->context.textArea->select_end_index = current_line * max_num + current_pos;
                }

                w->context.textArea->select_start_index = w->context.textArea->current_line * max_num + current_pos - 1;
            }
            else
            {
                w->context.textArea->select_start_index = -2;
                w->context.textArea->select_end_index = -2;
            }
            drawTextAreaWidget(win, index);
            updatePartWindow(win, w->size.x, w->size.y, w->size.width, w->size.height);
            return;
        }

        if (msg->params[0] >= 'a' && msg->params[0] <= 'z' && (msg->params[1] & 1) == 1)
        {
            for (int i = len; i >= (current_line * max_num + current_pos + 1); i--)
            {
                w->context.textArea->text[i] = w->context.textArea->text[i - 1];
            }
            w->context.textArea->text[(current_line * max_num + current_pos)] = msg->params[0] - 32;
            w->context.textArea->text[len + 1] = '\0';
            if (current_pos < max_num - 1)
            {
                w->context.textArea->current_pos++;
            }
            else
            {
                w->context.textArea->current_line++;
                w->context.textArea->current_pos = 0;
            }

            drawTextAreaWidget(win, index);
            updatePartWindow(win, w->size.x, w->size.y, w->size.width, w->size.height);
            return;
        }

        // shift key map
        if(msg->params[0]>=48 && msg->params[0]<=57 && (msg->params[1] & 1) ==1){
            msg->params[0] -= 17;
        }
        if(msg->params[0]>=91 && msg->params[0]<=93 && (msg->params[1] & 1) ==1){
            msg->params[0] +=32;
        }


        if (msg->params[0] == 0)
        {
            return;
        }

        // for (int i = len; i >= (current_line * max_num + current_pos + 1); i--)
        // {
        //     w->context.textArea->text[i] = w->context.textArea->text[i - 1];
        // }
        // w->context.textArea->text[(current_line * max_num + current_pos)] = msg->params[0];

        for (int i = len; i >= (w->context.textArea->insert_index + 1); i--)
        {
            w->context.textArea->text[i] = w->context.textArea->text[i - 1];
        }
        w->context.textArea->text[w->context.textArea->insert_index] = msg->params[0];

        w->context.textArea->text[len + 1] = '\0';

        if (current_pos < max_num - 1 && msg->params[0]!='\n')
        {
            w->context.textArea->current_pos++;
        }
        else
        {
            w->context.textArea->current_line++;
            w->context.textArea->current_pos = 0;
        }


        drawTextAreaWidget(win, index);
        updatePartWindow(win, w->size.x, w->size.y, w->size.width, w->size.height);
    }
    if (msg->msg_type == M_MOUSE_LEFT_CLICK)
    {
        int cursor_line = msg->params[1] / w->size.width;
        int cursor_pos = msg->params[0] / CHARACTER_WIDTH;

        printf(1, "cursor line: %d\n", cursor_line);
        printf(1, "cursor pos:%d\n", cursor_pos);
        printf(1, "len:%d\n", len);

        if ((cursor_line * max_num + cursor_pos) >= len)
        {
            return;
        }
        else
        {
            w->context.textArea->current_line = cursor_line;
            w->context.textArea->current_pos = cursor_pos;
        }
        drawTextAreaWidget(win, index);
        updatePartWindow(win, w->size.x, w->size.y, w->size.width, w->size.height);
    }
}

void fileListDoubleClickHandler(window *win, int index, message *msg)
{
    Widget *w = &(win->widgets[index]);

    int max_num_y = w->size.height / ICON_VIEW_SIZE;
    int max_num_x = w->size.width / ICON_VIEW_SIZE;
    int offset_y = w->size.height % ICON_VIEW_SIZE / max_num_y;
    int offset_x = w->size.width % ICON_VIEW_SIZE / max_num_x;
    int current_x = (msg->params[0] - offset_x) / ICON_VIEW_SIZE;
    int current_y = (msg->params[1] - offset_y) / ICON_VIEW_SIZE;
    int calcu_index;

    if (w->context.fileList->direction == 0)
    { // on the desktop
        calcu_index = current_x * max_num_y + current_y;
    }
    else
    {
        calcu_index = current_y * max_num_x + current_x;
    }
    if (calcu_index < w->context.fileList->file_num)
    {
        IconView *p = w->context.fileList->file_list;
        int i;
        for (i = 0; i < calcu_index; i++)
        {
            p = p->next;
        }
        char t[20];
        UI_suffix(t, p->text);
        if (strcmp(t, "bmp") == 0)
        {
            if (fork() == 0)
            {
                char *argv2[] = {"image_viewer", p->text, 0};
                exec(argv2[0], argv2);
                exit();
            }
        }
        else if (strcmp(t, "") == 0)
        {
            if (fork() == 0)
            {
                char *argv2[] = {p->text, 0};
                exec(argv2[0], argv2);
                exit();
            }
        }
        else if (strcmp(t, "txt") == 0)
        {
            if (fork() == 0)
            {
                char *argv2[] = {"Editor", p->text, 0};
                exec(argv2[0], argv2);
                exit();
            }
        }
    }
}

void mainLoop(window *win)
{
    message msg;
    while (1)
    {
        if (getmessage(win->handler, &msg))
        {
            if (msg.msg_type == WM_WINDOW_CLOSE)
            {
                UI_destroyWindow(win);
                break;
            }
            if (msg.msg_type == M_MOUSE_DBCLICK)
            {
                int i;
                for (i = 0; i < win->widget_number; i++)
                {
                    if (win->widgets[i].type == FILE_LIST)
                    {
                        win->widgets[i].context.fileList->onDoubleClick(win, i, &msg);
                        break;
                    }
                }
            }
            else if (msg.msg_type == M_KEY_DOWN)
            {
                int i;
                for (i = 0; i < win->widget_number; i++)
                {
                    if (win->widgets[i].type == TEXT_AREA)
                    {
                        win->widgets[i].context.textArea->onKeyDown(win, i, &msg);
                        break;
                    }
                }
            }
            else if (msg.msg_type == M_MOUSE_LEFT_CLICK)
            {
                int i;
                for (i = 0; i < win->widget_number; i++)
                {
                    if (win->widgets[i].type == BUTTON)
                    {
                        win->widgets[i].context.button->onLeftClick(win, i, &msg);
                        break;
                    }
                    if (win->widgets[i].type == TEXT_AREA)
                    {
                        win->widgets[i].context.textArea->onKeyDown(win, i, &msg);
                        break;
                    }
                }
            }
        }
    }
}
