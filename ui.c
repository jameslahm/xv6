#include "types.h"
#include "stat.h"
#include "color.h"
#include "msg.h"
#include "user.h"
#include "fcntl.h"
#include "ui.h"
#include "character.h"
#include "fs.h"
#include "cmd.h"
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
char shift_ascii_map[128] =
    {
        [0x30] 0x29, [0x31] 0x21, [0x32] 0x40, [0x33] 0x23, [0x34] 0x24, [0x35] 0x25, [0x36] 0x5E, [0x37] 0x26, [0x38] 0x2A, [0x39] 0x28, [0x2D] 0x5F, [0x3D] 0x2B, [0x5B] 0x7B, [0x5D] 0x7D, [0x5C] 0x7C, [0x3B] 0x3A, [0x27] 0x22, [0x2F] 0x3F, [0x2C] 0x3C, [0x2E] 0x3E, [0x60] 0x7E};

struct RGBA brown = {87, 144, 111, 255};
struct RGBA green = {185, 198, 190, 255};
struct RGBA blue = {175, 147, 113, 255};
struct RGBA pink = {184, 138, 185, 255};
struct RGBA orange = {141, 149, 186, 255};

void drawImageWidget(window *win, int index);
void drawLabelWidget(window *win, int index);
void drawButtonWidget(window *win, int index);
void drawInputWidget(window *win, int index);
void drawTextAreaWidget(window *win, int index);
void drawFileListWidget(window *win, int index);

void fileListDoubleClickHandler(window *win, int index, message *msg);
void textAreaKeyDownHandler(window *win, int index, message *msg);
void generateHighlightRGB(Widget *w);

void push_command(struct CommandStack *command_stack, enum CommandType type, int index, char *content, char *old_content, int isBatch)
{
    if (type == ADD)
    {
        command_stack->stack[++command_stack->stack_pos].type = ADD;
        command_stack->stack[command_stack->stack_pos].index = index;
        command_stack->stack[command_stack->stack_pos].isBatch = isBatch;
        char *buf = (char *)malloc(BUF_SIZE);
        memset(buf, 0, BUF_SIZE);
        strcpy(buf, content);
        command_stack->stack[command_stack->stack_pos].content = buf;
        command_stack->max_stack_pos = command_stack->stack_pos;
    }
    if (type == DEL)
    {
        command_stack->stack[++command_stack->stack_pos].type = DEL;
        command_stack->stack[command_stack->stack_pos].index = index;
        command_stack->stack[command_stack->stack_pos].isBatch = isBatch;
        char *buf = (char *)malloc(BUF_SIZE);
        memset(buf, 0, BUF_SIZE);
        strcpy(buf, content);
        command_stack->stack[command_stack->stack_pos].content = buf;
        command_stack->max_stack_pos = command_stack->stack_pos;
    }
}

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
    for (int i = 0; i < MAX_LONG_STRLEN; i++)
    {
        t->colors[i] = c;
    }
    t->current_pos = 0;
    t->current_line = 0;
    t->select_start_index = -2;
    t->select_end_index = -2;
    t->isCopying = 0;
    t->begin_line = 0;
    t->scale = 1;
    t->temp = 0;
    for (int i = 0; i < MAX_LONG_STRLEN; i++)
    {
        t->command_stack.stack[i].isBatch = 0;
    }
    memset(t->search_text, 0, BUF_SIZE);
    memset(t->replace_text, 0, BUF_SIZE);
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
    if (widget->context.fileList->file_list)
    {
        IconView *node = widget->context.fileList->file_list;
        for (int i = 0; i < widget->context.fileList->file_num; i++)
        {
            IconView *tmp = node->next;
            free(node);
            node = tmp;
        }
        widget->context.fileList->file_list = 0;
    }
    printf(1, "FileList: %d\n", widget->context.fileList->file_list);
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
        printf(1, "%s\n", tmpName);
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

int drawCharacterWithBgWithScale(window *win, int x, int y, char ch, RGBA color, RGBA bg, int scale)
{
    int i, j;
    RGB *t;
    int ord = ch - 0x20;
    if (ord < 0 || ord >= (CHARACTER_NUMBER))
    {
        return -1;
    }
    for (i = 0; i < CHARACTER_HEIGHT * scale; i++)
    {
        if (y + i > win->height)
        {
            break;
        }
        if (y + i < 0)
        {
            continue;
        }
        for (j = 0; j < CHARACTER_WIDTH * scale; j++)
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
            if (character[ord][i / scale][j / scale] == 1)
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

int drawCharacterWicthScale(window *win, int x, int y, char ch, RGBA color, int scale)
{
    int i, j;
    RGB *t;
    int ord = ch - 0x20;
    if (ord < 0 || ord >= (CHARACTER_NUMBER))
    {
        return -1;
    }
    for (i = 0; i < CHARACTER_HEIGHT * scale; i++)
    {
        if (y + i > win->height)
        {
            break;
        }
        if (y + i < 0)
        {
            continue;
        }
        for (j = 0; j < CHARACTER_WIDTH * scale; j++)
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
            if (character[ord][i / scale][j / scale] == 1)
            {
                drawPointAlpha(t, color);
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
        t = win->window_buf + y * win->width + x + width;
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
    printf(1, "Repaint!!!\n");
    printf(1, "%s\n", w->context.textArea->text);

    int matchIndex[BUF_SIZE];
    int matchNums = 0;

    if (w->context.textArea->isSearching)
    {
        char *text = w->context.textArea->text;
        char *search_text = w->context.textArea->search_text;
        for (int i = 0; i <= strlen(text) - strlen(search_text); i++)
        {
            int j = 0;
            for (; j < strlen(search_text); j++)
            {
                if (text[i + j] != search_text[j])
                {
                    break;
                }
            }
            if (j == strlen(search_text) && j != 0)
            {
                matchIndex[matchNums++] = i;
            }
        }
    }
    int currentMatchIndex = 0;

    // printf(1,"cursor_line: %d cursor_pos: %d",w->context.textArea->current_line,w->context.textArea->current_pos);

    for (i = 0; w->context.textArea->text[i] || (current_x == w->context.textArea->current_pos && current_y == w->context.textArea->current_line); i++)
    {
        char ch = w->context.textArea->text[i];
        int newline = 0;
        int isCursor = 0;
        struct RGBA color = w->context.textArea->colors[i];
        if (current_x == w->context.textArea->current_pos && current_y == w->context.textArea->current_line)
        {
            ch = 95 + 0x20;
            w->context.textArea->insert_index = i;
            isCursor = 1;
            i--;
        }
        else if (w->context.textArea->text[i] == '\n')
        {
            newline = 1;
            current_y++;
            current_x = 0;
            if (current_y >= max_line)
            {
                break;
            }
        }
        int scale = w->context.textArea->scale;

        if (!isCursor && ((i >= w->context.textArea->select_start_index && i <= w->context.textArea->select_end_index) ||
                          (currentMatchIndex < matchNums && (matchIndex[currentMatchIndex] <= i && i <= matchIndex[currentMatchIndex] + strlen(w->context.textArea->search_text) - 1))))
        {
            if (matchIndex[currentMatchIndex] + strlen(w->context.textArea->search_text) - 1 == i)
            {
                currentMatchIndex++;
            }
            drawCharacterWithBgWithScale(win, w->size.x + current_x * CHARACTER_WIDTH * scale, w->size.y + current_y * CHARACTER_HEIGHT * scale,
                                         ch, color, gray, w->context.textArea->scale);
        }
        else
        {
            drawCharacterWicthScale(win, w->size.x + current_x * CHARACTER_WIDTH * scale, w->size.y + current_y * CHARACTER_HEIGHT * scale,
                                    ch, color, w->context.textArea->scale);
        }
        if (!newline)
        {
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

    struct RGB black = (struct RGB){0, 0, 0};
    struct RGBA blackA = (struct RGBA){0, 0, 0, 255};
    if (w->context.textArea->isSearching)
    {
        drawRect(win, black, 650, 10, 100, CHARACTER_HEIGHT + 2);
        char *search_text = w->context.textArea->search_text;
        for (int i = 0; search_text[i] && i < BUF_SIZE; i++)
        {
            drawCharacter(win, 650 + i * CHARACTER_WIDTH, 11,
                          search_text[i], blackA);
        }
        char *replace_text = w->context.textArea->replace_text;
        drawRect(win, black, 650, 30, 100, CHARACTER_HEIGHT + 2);
        for (int i = 0; replace_text[i] && i < BUF_SIZE; i++)
        {
            drawCharacter(win, 650 + i * CHARACTER_WIDTH, 31,
                          replace_text[i], blackA);
        }
    }

    if (w->context.textArea->isInTerminal)
    {
        drawRect(win, black, 0, 440, 769, 130);
        char *cmd = w->context.textArea->cmd;
        char *cmd_res = w->context.textArea->cmd_res;
        int current_x = 0;
        int current_y = 0;
        for (int i = 0; i < strlen(cmd_res); i++)
        {
            if (cmd_res[i] == '\n')
            {
                current_y++;
                current_x = 0;
            }
            else
            {
                drawCharacter(win, 1 + CHARACTER_WIDTH * (current_x), 441 + CHARACTER_HEIGHT * current_y,
                              cmd_res[i], blackA);
                current_x++;
            }
        }
        drawCharacter(win, 1, 441 + current_y * CHARACTER_HEIGHT,
                      '>', blackA);
        drawCharacter(win, 1 + CHARACTER_WIDTH, 441 + current_y * CHARACTER_HEIGHT,
                      '>', blackA);
        for (int i = 0; i < strlen(cmd); i++)
        {
            drawCharacter(win, 1 + CHARACTER_WIDTH * (i + 2), 441 + current_y * CHARACTER_HEIGHT,
                          cmd[i], blackA);
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

void generateHighlightRGB(Widget *w)
{
    char *text = w->context.textArea->text;
    int len = strlen(text);
    RGBA *colors = w->context.textArea->colors;
    for (int i = 0; i < len; i++)
    {
        colors[i] = w->context.textArea->color;
    }

    for (int i = 0; i < len && text[i]; i++)
    {
        // printf(1, "\e[1;30m%d%d%d\e[0;32m| \e[0m", (i + 1) / 100, (i + 1) % 100 / 10, (i + 1) % 10);
        // find next \n
        if (text[i] == '\n')
        {
            continue;
        }

        int newlineIndex = len;
        for (int j = i; j < len; j++)
        {
            if (text[j] == '\n')
            {
                newlineIndex = j;
                break;
            }
        }

        // annotation
        if (text[i] == '/' && text[i + 1] == '/')
        {
            for (int j = i; j < newlineIndex; j++)
            {
                colors[j] = brown;
                i++;
            }
            continue;
        }

        while (i < newlineIndex)
        {
            // numbers
            if (text[i] >= '0' && text[i] <= '9')
            {
                colors[i] = green;
                i += 1;
            }

            // string
            // TODO: fix possible segement fault
            else if (text[i] == '"')
            {
                int start = i;
                int end = i + 1;
                for (; text[end] != '"' && text[end] != '\0'; end++)
                    ;
                for (int i = start; i <= end; i++)
                {
                    colors[i] = orange;
                }
                i = end;
            }

            // single char
            else if (text[i] == '\'')
            {
                if (i + 1 < len)
                    colors[i + 1] = orange;
                if (i + 2 < len)
                    colors[i + 2] = orange;
                i += 3;
            }

            // int
            else if ((len - i) >= 3 && text[i] == 'i' && text[i + 1] == 'n' && text[i + 2] == 't')
            {
                colors[i] = blue;
                colors[i + 1] = blue;
                colors[i + 2] = blue;
                i += 3;
            }

            // long
            else if ((len - i) >= 4 && text[i] == 'l' && text[i + 1] == 'o' && text[i + 2] == 'n' && text[i + 3] == 'g')
            {
                colors[i] = blue;
                colors[i + 1] = blue;
                colors[i + 2] = blue;
                colors[i + 3] = blue;
                i += 4;
            }

            // double
            else if ((len - i) >= 6 && text[i] == 'd' && text[i + 1] == 'o' && text[i + 2] == 'u' && text[i + 3] == 'b' && text[i + 4] == 'l' && text[i + 5] == 'e')
            {
                colors[i] = blue;
                colors[i + 1] = blue;
                colors[i + 2] = blue;
                colors[i + 3] = blue;
                colors[i + 4] = blue;
                colors[i + 5] = blue;
                i += 6;
            }

            // float
            else if ((len - i) >= 5 && text[i] == 'f' && text[i + 1] == 'l' && text[i + 2] == 'o' && text[i + 3] == 'a' && text[i + 4] == 't')
            {
                colors[i] = blue;
                colors[i + 1] = blue;
                colors[i + 2] = blue;
                colors[i + 3] = blue;
                colors[i + 4] = blue;
                i += 5;
            }

            // char
            else if ((len - i) >= 4 && text[i] == 'c' && text[i + 1] == 'h' && text[i + 2] == 'a' && text[i + 3] == 'r')
            {
                colors[i] = blue;
                colors[i + 1] = blue;
                colors[i + 2] = blue;
                colors[i + 3] = blue;
                i += 4;
            }

            // if
            else if ((len - i) >= 2 && text[i] == 'i' && text[i + 1] == 'f')
            {
                colors[i] = pink;
                colors[i + 1] = pink;
                i += 2;
            }

            // else
            else if ((len - i) >= 4 && text[i] == 'e' && text[i + 1] == 'l' && text[i + 2] == 's' && text[i + 3] == 'e')
            {
                colors[i] = pink;
                colors[i + 1] = pink;
                colors[i + 2] = pink;
                colors[i + 3] = pink;
                i += 4;
            }

            // else if
            else if ((len - i) >= 7 && text[i] == 'e' && text[i + 1] == 'l' && text[i + 2] == 's' && text[i + 3] == 'e' && text[i + 4] == ' ' && text[i + 5] == 'i' && text[i + 6] == 'f')
            {
                colors[i] = pink;
                colors[i + 1] = pink;
                colors[i + 2] = pink;
                colors[i + 3] = pink;
                colors[i + 4] = pink;
                colors[i + 5] = pink;
                colors[i + 6] = pink;
                i += 7;
            }

            // for
            else if ((len - i) >= 3 && text[i] == 'f' && text[i + 1] == 'o' && text[i + 2] == 'r')
            {
                colors[i] = pink;
                colors[i + 1] = pink;
                colors[i + 2] = pink;
                i += 3;
            }

            // while
            else if ((len - i) >= 5 && text[i] == 'w' && text[i + 1] == 'h' && text[i + 2] == 'i' && text[i + 3] == 'l' && text[i + 4] == 'e')
            {
                colors[i] = pink;
                colors[i + 1] = pink;
                colors[i + 2] = pink;
                colors[i + 3] = pink;
                colors[i + 4] = pink;
                i += 5;
            }

            // static
            else if ((len - i) >= 6 && text[i] == 's' && text[i + 1] == 't' && text[i + 2] == 'a' && text[i + 3] == 't' && text[i + 4] == 'i' && text[i + 5] == 'c')
            {
                colors[i] = pink;
                colors[i + 1] = pink;
                colors[i + 2] = pink;
                colors[i + 3] = pink;
                colors[i + 4] = pink;
                colors[i + 5] = pink;
                i += 6;
            }

            // const
            else if ((len - i) >= 5 && text[i] == 'c' && text[i + 1] == 'o' && text[i + 2] == 'n' && text[i + 3] == 's' && text[i + 4] == 't')
            {
                colors[i] = pink;
                colors[i + 1] = pink;
                colors[i + 2] = pink;
                colors[i + 3] = pink;
                colors[i + 4] = pink;
                i += 5;
            }

            else if ((len - i) >= 8 && text[i] == 'c' && text[i + 1] == 'o' && text[i + 2] == 'n' && text[i + 3] == 't' && text[i + 4] == 'i' && text[i + 5] == 'n' && text[i + 6] == 'u' && text[i + 7] == 'e')
            {
                colors[i] = pink;
                colors[i + 1] = pink;
                colors[i + 2] = pink;
                colors[i + 3] = pink;
                colors[i + 4] = pink;
                colors[i + 5] = pink;
                colors[i + 6] = pink;
                colors[i + 7] = pink;
                i += 8;
            }
            else if ((len - i) >= 6 && text[i] == 'r' && text[i + 1] == 'e' && text[i + 2] == 't' && text[i + 3] == 'u' && text[i + 4] == 'r' && text[i + 5] == 'n')
            {
                colors[i] = pink;
                colors[i + 1] = pink;
                colors[i + 2] = pink;
                colors[i + 3] = pink;
                colors[i + 4] = pink;
                colors[i + 5] = pink;
                i += 6;
            }

            else if ((len - i) >= 5 && text[i] == 'b' && text[i + 1] == 'r' && text[i + 2] == 'e' && text[i + 3] == 'a' && text[i + 4] == 'k')
            {
                colors[i] = pink;
                colors[i + 1] = pink;
                colors[i + 2] = pink;
                colors[i + 3] = pink;
                colors[i + 4] = pink;
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

        printf(1, "DEBUG %x\n", msg->params[0]);

        if (msg->params[0] == 0x1b)
        {
            w->context.textArea->isSearching = 0;
            w->context.textArea->isInTerminal = 0;
            memset(w->context.textArea->search_text, 0, BUF_SIZE);
            memset(w->context.textArea->replace_text, 0, BUF_SIZE);
            memset(w->context.textArea->cmd, 0, BUF_SIZE);
            w->context.textArea->select_start_index = -2;
            w->context.textArea->select_end_index = -2;
            drawTextAreaWidget(win, index);
            updatePartWindow(win, w->size.x, w->size.y, w->size.width, w->size.height);
            return;
        }

        if (w->context.textArea->isSearching == 1)
        {
            char *search_text = w->context.textArea->search_text;
            if (msg->params[0] == '\b')
            {
                search_text[strlen(search_text) - 1] = '\0';
            }
            else if (msg->params[0] == 0x9)
            {
                w->context.textArea->isSearching = 2;
            }
            else
            {
                if ((msg->params[1] & 1) == 1)
                {
                    if (msg->params[0] >= 'a' && msg->params[0] <= 'z')
                    {
                        msg->params[0] -= 32;
                    }
                    else
                    {
                        if (msg->params[0])
                        {
                            msg->params[0] = shift_ascii_map[msg->params[0]];
                        }
                    }
                }
                search_text[strlen(search_text)] = msg->params[0];
            }
            drawTextAreaWidget(win, index);
            updatePartWindow(win, w->size.x, w->size.y, w->size.width, w->size.height);
            return;
        }
        if (w->context.textArea->isSearching == 2)
        {
            char *replace_text = w->context.textArea->replace_text;
            if (msg->params[0] == '\b')
            {
                replace_text[strlen(replace_text) - 1] = '\0';
            }
            else if (msg->params[0] == 0x9 && (msg->params[1] & 1) == 1)
            {
                w->context.textArea->isSearching = 1;
            }
            else if (msg->params[0] == '\n')
            {

                char *text = w->context.textArea->text;
                char *search_text = w->context.textArea->search_text;
                while (1)
                {
                    int flag = 0;
                    for (int i = 0; i <= strlen(text) - strlen(search_text); i++)
                    {
                        int j = 0;
                        for (; j < strlen(search_text); j++)
                        {
                            if (text[i + j] != search_text[j])
                            {
                                break;
                            }
                        }
                        if (j == strlen(search_text) && j != 0)
                        {
                            flag = 1;
                            int text_len = strlen(text);
                            for (int k = i; k < text_len - strlen(search_text); k++)
                            {
                                w->context.textArea->text[k] = w->context.textArea->text[k + strlen(search_text)];
                            }
                            for (int k = text_len - strlen(search_text); k < text_len; k++)
                            {
                                w->context.textArea->text[k] = '\0';
                            }
                            push_command(&w->context.textArea->command_stack, DEL, i, search_text, 0, 1);

                            text_len = strlen(text);

                            for (int k = text_len + strlen(replace_text) - 1; k >= i + strlen(replace_text); k--)
                            {
                                w->context.textArea->text[k] = w->context.textArea->text[k - strlen(replace_text)];
                            }

                            for (int k = i; k < i + strlen(replace_text); k++)
                            {
                                w->context.textArea->text[k] = replace_text[k - i];
                                // printf(1,"Replace %c\n",replace_text[k - i]);
                            }
                            push_command(&w->context.textArea->command_stack, ADD, i, replace_text, 0, 1);
                            break;
                        }
                    }
                    if (!flag)
                    {
                        break;
                    }
                }
            }
            else
            {
                if ((msg->params[1] & 1) == 1)
                {
                    if (msg->params[0] >= 'a' && msg->params[0] <= 'z')
                    {
                        msg->params[0] -= 32;
                    }
                    else
                    {
                        if (msg->params[0])
                        {
                            msg->params[0] = shift_ascii_map[msg->params[0]];
                        }
                    }
                }
                replace_text[strlen(replace_text)] = msg->params[0];
            }
            drawTextAreaWidget(win, index);
            updatePartWindow(win, w->size.x, w->size.y, w->size.width, w->size.height);
            return;
        }
        if (w->context.textArea->isInTerminal)
        {
            char *cmd = w->context.textArea->cmd;
            char *cmd_res = w->context.textArea->cmd_res;
            if (msg->params[0] == '\b')
            {
                cmd[strlen(cmd) - 1] = '\0';
            }
            else if (msg->params[0] == '\n')
            {
                int p[2];
                if (pipe(p) < 0)
                {
                    printf(1, "Pipe Error\n");
                }
                if (fork1() == 0)
                {
                    close(1);
                    close(2);
                    dup(p[1]);
                    dup(p[1]);
                    close(p[0]);
                    close(p[1]);
                    runcmd(parsecmd(cmd));
                    close(1);
                    close(2);
                }
                close(p[1]);
                wait();
                printf(1, "Complete!!!\n");

                cmd_res[strlen(cmd_res)] = '>';
                cmd_res[strlen(cmd_res)] = '>';
                strcpy(cmd_res + strlen(cmd_res), cmd);
                cmd_res[strlen(cmd_res)] = '\n';
                char c;
                int i = strlen(cmd_res);
                while (1)
                {
                    int cc = read(p[0], &c, 1);
                    if (cc < 1)
                        break;
                    cmd_res[i++] = c;
                }
                memset(cmd, 0, BUF_SIZE);
                printf(1, "Cmd Res: %s", cmd_res);
                printf(1, "Cmd: \n", cmd);
            }
            else
            {
                if ((msg->params[1] & 1) == 1)
                {
                    if (msg->params[0] >= 'a' && msg->params[0] <= 'z')
                    {
                        msg->params[0] -= 32;
                    }
                    else
                    {
                        if (msg->params[0])
                        {
                            msg->params[0] = shift_ascii_map[msg->params[0]];
                        }
                    }
                }
                cmd[strlen(cmd)] = msg->params[0];
            }
            drawTextAreaWidget(win, index);
            updatePartWindow(win, w->size.x, w->size.y, w->size.width, w->size.height);
            return;
        }

        if (msg->params[0] == '\b')
        {
            if (len < 1)
            {
                return;
            }

            if (w->context.textArea->select_start_index < 0)
            {

                int insert_index = w->context.textArea->insert_index;
                char temp[2] = {0};
                temp[0] = w->context.textArea->text[insert_index - 1];
                push_command(&w->context.textArea->command_stack, DEL, insert_index - 1, temp, 0, 0);
                for (int i = w->context.textArea->insert_index - 1; i < len - 1; i++)
                {
                    w->context.textArea->text[i] = w->context.textArea->text[i + 1];
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
                        int current_line = 0;
                        int current_pos = 0;
                        for (int i = 0; i < w->context.textArea->insert_index - 1; i++)
                        {
                            if (w->context.textArea->text[i] == '\n')
                            {
                                current_line++;
                                current_pos = 0;
                            }
                            else
                            {
                                current_pos++;
                            }
                        }
                        w->context.textArea->current_line = current_line;
                        w->context.textArea->current_pos = current_pos;
                    }
                }
            }
            else{
                int selectTextLength = w->context.textArea->select_end_index - w->context.textArea->select_start_index +1;
                int newLineNumbers = 0;
                int leftPos = 0;
                char temp[BUF_SIZE];
                memset(temp,0,BUF_SIZE);
                for (int i = w->context.textArea->select_start_index - 1; i >= 0; i--)
                {
                    if (w->context.textArea->text[i] == '\n')
                    {
                        break;
                    }
                    leftPos++;
                }
                for (int i = 0; i < selectTextLength; i++)
                {
                    temp[i] = w->context.textArea->text[w->context.textArea->select_start_index + i];
                    if (temp[i] == '\n')
                    {
                        newLineNumbers++;
                    }
                }
                push_command(&w->context.textArea->command_stack, DEL, w->context.textArea->select_start_index, temp, 0, 0);
                for (int i = w->context.textArea->select_start_index; i < len - selectTextLength; i++)
                {
                    w->context.textArea->text[i] = w->context.textArea->text[i + selectTextLength];
                }

                if(w->context.textArea->insert_index>w->context.textArea->select_end_index){
                    w->context.textArea->current_line -= newLineNumbers;
                }
                w->context.textArea->current_pos = leftPos;
                printf(1, "current_line: %d current_pos: %d", w->context.textArea->current_line, w->context.textArea->current_pos);

                w->context.textArea->select_start_index = -2;
                w->context.textArea->select_end_index = -2;

                w->context.textArea->text[len - selectTextLength] = '\0';
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
                int insert_index = w->context.textArea->insert_index;
                int copyTextLength = w->context.textArea->copy_end_index - w->context.textArea->copy_start_index + 1;

                int newLineNumbers = 0;
                int leftPos = 0;

                push_command(&w->context.textArea->command_stack, ADD, insert_index, w->context.textArea->temp, 0, 0);

                for (int i = len + copyTextLength - 1; i >= insert_index + copyTextLength; i--)
                {
                    w->context.textArea->text[i] = w->context.textArea->text[i - copyTextLength];
                }
                w->context.textArea->text[len + copyTextLength] = '\0';
                for (int i = insert_index; i < insert_index + copyTextLength; i++)
                {
                    int j = i - insert_index;
                    w->context.textArea->text[i] = w->context.textArea->temp[j];
                    leftPos++;
                    if (w->context.textArea->temp[j] == '\n')
                    {
                        newLineNumbers++;
                        leftPos = 0;
                    }
                }

                // w->context.textArea->copy_start_index = w->context.textArea->copy_end_index = -2;
                w->context.textArea->current_line += newLineNumbers;
                w->context.textArea->current_pos = newLineNumbers == 0 ? w->context.textArea->current_pos + leftPos : leftPos;

                // printf(1,"current_line: %d current_pos: %d",w->context.textArea->current_line,w->context.textArea->current_pos);
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

                int newLineNumbers = 0;
                int leftPos = 0;
                for (int i = w->context.textArea->copy_start_index - 1; i >= 0; i--)
                {
                    if (w->context.textArea->text[i] == '\n')
                    {
                        break;
                    }
                    leftPos++;
                }
                for (int i = 0; i < copyTextLength; i++)
                {
                    w->context.textArea->temp[i] = w->context.textArea->text[w->context.textArea->copy_start_index + i];
                    if (w->context.textArea->temp[i] == '\n')
                    {
                        newLineNumbers++;
                    }
                }
                push_command(&w->context.textArea->command_stack, DEL, w->context.textArea->copy_start_index, w->context.textArea->temp, 0, 0);
                for (int i = w->context.textArea->copy_start_index; i < len - copyTextLength; i++)
                {
                    w->context.textArea->text[i] = w->context.textArea->text[i + copyTextLength];
                }

                if(w->context.textArea->insert_index>w->context.textArea->select_end_index){
                    w->context.textArea->current_line -= newLineNumbers;
                }
                w->context.textArea->current_pos = leftPos;
                printf(1, "current_line: %d current_pos: %d", w->context.textArea->current_line, w->context.textArea->current_pos);

                w->context.textArea->select_start_index = -2;
                w->context.textArea->select_end_index = -2;

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

            if (msg->params[0] == 'z' && (msg->params[1] & 1) == 1)
            {
                while (1)
                {
                    struct CommandStack *command_stack = &w->context.textArea->command_stack;
                    command_stack->stack_pos++;
                    struct Command *command = &command_stack->stack[command_stack->stack_pos];
                    int text_len = strlen(w->context.textArea->text);
                    if (command->type == ADD)
                    {
                        for (int i = command->index + strlen(command->content); i < text_len + strlen(command->content); i++)
                        {
                            w->context.textArea->text[i] = w->context.textArea->text[i - strlen(command->content)];
                        }
                        for (int i = command->index; i < command->index + strlen(command->content); i++)
                        {
                            w->context.textArea->text[i] = command->content[i - command->index];
                        }
                        w->context.textArea->current_pos += strlen(command->content);
                    }
                    if (command->type == DEL)
                    {
                        for (int i = command->index; i < text_len; i++)
                        {
                            w->context.textArea->text[i] = w->context.textArea->text[i + strlen(command->content)];
                        }
                        for (int i = text_len - strlen(command->content); i < text_len; i++)
                        {
                            w->context.textArea->text[i] = '\0';
                        }
                        w->context.textArea->current_pos -= strlen(command->content);
                    }

                    if (command_stack->stack_pos + 1 > command_stack->max_stack_pos)
                    {
                        break;
                    }

                    struct Command *suc_command = &command_stack->stack[command_stack->stack_pos + 1];
                    if (command->isBatch == 1 && suc_command->isBatch == 1)
                    {
                        continue;
                    }
                    else
                    {
                        break;
                    }
                }
                printf(1, "%s\n", w->context.textArea->text);
                drawTextAreaWidget(win, index);
                updatePartWindow(win, w->size.x, w->size.y, w->size.width, w->size.height);
                return;
            }

            if (msg->params[0] == 'z')
            {
                while (1)
                {
                    struct CommandStack *command_stack = &w->context.textArea->command_stack;
                    struct Command *command = &command_stack->stack[command_stack->stack_pos];
                    command_stack->stack_pos--;
                    int text_len = strlen(w->context.textArea->text);
                    if (command->type == ADD)
                    {
                        for (int i = command->index; i < text_len; i++)
                        {
                            w->context.textArea->text[i] = w->context.textArea->text[i + strlen(command->content)];
                        }
                        for (int i = text_len - strlen(command->content); i < text_len; i++)
                        {
                            w->context.textArea->text[i] = '\0';
                        }
                        // TODO: fix newline
                        w->context.textArea->current_pos -= strlen(command->content);
                    }
                    if (command->type == DEL)
                    {
                        for (int i = command->index + strlen(command->content); i < text_len + strlen(command->content); i++)
                        {
                            w->context.textArea->text[i] = w->context.textArea->text[i - strlen(command->content)];
                        }
                        for (int i = command->index; i < command->index + strlen(command->content); i++)
                        {
                            w->context.textArea->text[i] = command->content[i - command->index];
                        }
                        w->context.textArea->current_pos += strlen(command->content);
                    }

                    if (command_stack->stack_pos < 0)
                    {
                        break;
                    }

                    struct Command *pre_command = &command_stack->stack[command_stack->stack_pos];
                    if (command->isBatch == 1 && pre_command->isBatch == 1)
                    {
                        continue;
                    }
                    else
                    {
                        break;
                    }
                }
                drawTextAreaWidget(win, index);
                updatePartWindow(win, w->size.x, w->size.y, w->size.width, w->size.height);
                return;
            }

            if (msg->params[0] == 'f')
            {
                w->context.textArea->isSearching = 1;
                drawTextAreaWidget(win, index);
                updatePartWindow(win, w->size.x, w->size.y, w->size.width, w->size.height);
                return;
            }

            if (msg->params[0] == '=')
            {
                w->context.textArea->scale += 1;
                drawTextAreaWidget(win, index);
                updatePartWindow(win, w->size.x, w->size.y, w->size.width, w->size.height);
                return;
            }
            if (msg->params[0] == '-')
            {
                w->context.textArea->scale -= 1;
                if (w->context.textArea->scale == 0)
                {
                    w->context.textArea->scale = 1;
                }
                drawTextAreaWidget(win, index);
                updatePartWindow(win, w->size.x, w->size.y, w->size.width, w->size.height);
                return;
            }
            if (msg->params[0] == 's')
            {
                win->widgets[1].context.button->onLeftClick(win, 1, 0);
            }

            if (msg->params[0] == '`')
            {
                w->context.textArea->isInTerminal = 1;
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
                int insert_index = w->context.textArea->insert_index;
                w->context.textArea->select_start_index = insert_index - current_pos;
                w->context.textArea->select_end_index = insert_index;
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
            int insert_index = w->context.textArea->insert_index;
            if ((msg->params[1] & 1) == 1)
            {
                w->context.textArea->select_start_index = insert_index;
                // TODO: fix newline
                w->context.textArea->select_end_index = len - 1;
            }
            else
            {
                w->context.textArea->select_start_index = -2;
                w->context.textArea->select_end_index = -2;
            }
            // TODO: fix newline
            for (int i = insert_index; i < len; i++)
            {
                if (w->context.textArea->text[i] == '\n')
                {
                    break;
                }
                w->context.textArea->current_pos++;
            }
            drawTextAreaWidget(win, index);
            updatePartWindow(win, w->size.x, w->size.y, w->size.width, w->size.height);
            return;
        }

        // Up Down Left Right
        if (msg->params[0] == KEY_LF)
        {
            int insert_index = w->context.textArea->insert_index;
            if (current_pos >= 1)
            {
                w->context.textArea->current_pos--;
            }
            else
            {
                if (current_line >= 1)
                {
                    w->context.textArea->current_line--;
                    int beforeNewline = 0;
                    for (int i = insert_index - 2; i >= 0; i--)
                    {
                        if (w->context.textArea->text[i] == '\n')
                        {
                            break;
                        }
                        beforeNewline++;
                    }

                    w->context.textArea->current_pos = beforeNewline;
                }
            }
            if ((msg->params[1] & 1) == 1)
            {
                int insert_index = w->context.textArea->insert_index;
                if (w->context.textArea->select_start_index < 0)
                {

                    w->context.textArea->select_start_index = insert_index - 1;
                    w->context.textArea->select_end_index = w->context.textArea->select_start_index;
                }
                else
                {
                    if (w->context.textArea->select_start_index < insert_index)
                    {
                        w->context.textArea->select_end_index -= 1;
                    }
                    else
                    {
                        w->context.textArea->select_start_index -= 1;
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
            char *text = w->context.textArea->text;
            int insert_index = w->context.textArea->insert_index;
            if ((insert_index) < len)
            {
                if (text[insert_index] == '\n')
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
                    w->context.textArea->select_start_index = insert_index;
                    w->context.textArea->select_end_index = w->context.textArea->select_start_index;
                }
                else
                {
                    if (w->context.textArea->select_start_index < insert_index)
                    {
                        w->context.textArea->select_end_index += 1;
                    }
                    else
                    {
                        w->context.textArea->select_start_index += 1;
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
            int insert_index = w->context.textArea->insert_index;

            if (w->context.textArea->current_line >= 1)
            {
                w->context.textArea->current_line--;
            }
            if ((msg->params[1] & 1) == 1)
            {
                if (w->context.textArea->select_end_index < 0)
                {
                    w->context.textArea->select_end_index = insert_index - 1;
                }

                // w->context.textArea->select_start_index = insert_index;
                // TODO: fix
                int nextNewlineIndex = 0;
                char *text = w->context.textArea->text;
                for (int k = insert_index - 1; k >= 0; k--)
                {
                    if (text[k] == '\n')
                    {
                        nextNewlineIndex = k;
                        break;
                    }
                }
                int beforeNewlineIndex = 0;
                for (int k = nextNewlineIndex - 1; k >= 0; k--)
                {
                    if (text[k] == '\n')
                    {
                        beforeNewlineIndex = k;
                        break;
                    }
                }
                w->context.textArea->select_start_index = beforeNewlineIndex + insert_index - nextNewlineIndex;
                if (beforeNewlineIndex == 0 && text[beforeNewlineIndex] != '\n')
                {
                    w->context.textArea->select_start_index -= 1;
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
        if (msg->params[0] == KEY_DN)
        {
            int insert_index = w->context.textArea->insert_index;
            w->context.textArea->current_line++;
            if ((msg->params[1] & 1) == 1)
            {
                if (w->context.textArea->select_start_index < 0)
                {
                    w->context.textArea->select_start_index = insert_index;
                }
                // w->context.textArea->select_start_index = insert_index -1;
                // TODO: fix
                int beforeNewlineIndex = 0;
                char *text = w->context.textArea->text;
                for (int k = insert_index - 1; k >= 0; k--)
                {
                    if (text[k] == '\n')
                    {
                        beforeNewlineIndex = k;
                        break;
                    }
                }
                int nextNewlineIndex = 0;
                for (int k = insert_index; k < len; k++)
                {
                    if (text[k] == '\n')
                    {
                        nextNewlineIndex = k;
                        break;
                    }
                }
                w->context.textArea->select_end_index = nextNewlineIndex + insert_index - beforeNewlineIndex - 1;
                if (beforeNewlineIndex == 0 && text[beforeNewlineIndex] != '\n')
                {
                    w->context.textArea->select_end_index += 1;
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

        if (msg->params[0] >= 'a' && msg->params[0] <= 'z' && (msg->params[1] & 1) == 1)
        {
            int insert_index = w->context.textArea->insert_index;
            char temp[2] = {0};
            temp[0] = msg->params[0] - 32;
            push_command(&w->context.textArea->command_stack, ADD, insert_index, temp, 0, 0);
            for (int i = len; i >= (insert_index + 1); i--)
            {
                w->context.textArea->text[i] = w->context.textArea->text[i - 1];
            }
            w->context.textArea->text[(insert_index)] = msg->params[0] - 32;
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
        else if (msg->params[0] && (msg->params[1] & 1) == 1)
        {
            msg->params[0] = shift_ascii_map[msg->params[0]];
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

        char temp[2] = {0};
        temp[0] = msg->params[0];
        push_command(&w->context.textArea->command_stack, ADD, w->context.textArea->insert_index, temp, 0, 0);

        for (int i = len; i >= (w->context.textArea->insert_index + 1); i--)
        {
            w->context.textArea->text[i] = w->context.textArea->text[i - 1];
        }
        w->context.textArea->text[w->context.textArea->insert_index] = msg->params[0];

        w->context.textArea->text[len + 1] = '\0';

        if (current_pos < max_num - 1 && msg->params[0] != '\n')
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
        int cursor_line = msg->params[1] / (CHARACTER_HEIGHT * w->context.textArea->scale);
        int cursor_pos = msg->params[0] / (CHARACTER_WIDTH * w->context.textArea->scale);

        printf(1, "cursor line: %d\n", cursor_line);
        printf(1, "cursor pos:%d\n", cursor_pos);
        w->context.textArea->current_line = cursor_line;
        w->context.textArea->current_pos = cursor_pos;
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
        printf(1, "Invoke!!!\n");
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

int isInRect(int x, int y, widget_size size)
{
    return x >= size.x && x <= size.x + size.width && y >= size.y && y <= size.y + size.height;
}

void mainLoop(window *win)
{
    message msg;
    while (1)
    {
        if (getmessage(win->handler, &msg))
        {
            printf(1, "MSG: %d\n", msg.msg_type);
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
                        win->widgets[i].context.fileList->file_num = 0;
                        UI_ls(win->widgets[i].context.fileList->path, &win->widgets[i]);
                        drawAllWidget(win);
                        updatewindow(win->handler, 0, 0, win->width, win->height);
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
                        if (isInRect(msg.params[0], msg.params[1], win->widgets[i].size))
                            win->widgets[i].context.button->onLeftClick(win, i, &msg);
                    }
                    if (win->widgets[i].type == TEXT_AREA)
                    {
                        if (isInRect(msg.params[0], msg.params[1], win->widgets[i].size))
                            win->widgets[i].context.textArea->onKeyDown(win, i, &msg);
                    }
                }
            }
        }
    }
}
