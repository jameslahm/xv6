#ifndef __ASSEMBLER__
#define MAX_WIDTH 800
#define MAX_HEIGHT 600

#define BUF_SIZE 100

#define MAX_WIDGET 10
#define MAX_SHORT_STRLEN 20
#define MAX_LONG_STRLEN 512

struct Message;
struct Window;
typedef void (*Handler)(struct Window *win, int index, Message *msg);
typedef void (*painter)(struct Window *win, int index);

#define IMAGE 0
#define TEXT_AREA 1
#define FILE_LIST 2

#define FILE_TYPE_NUM 5
#define TEXT_FILE 0
#define BMP_FILE 1
#define EXEC_FILE 2
#define FOLDER_FILE 3
#define UNKNOWN_FILE 4

#define ICON_IMG_SIZE 64
#define ICON_VIEW_SIZE 90

typedef struct WidgetSize
{
    int x;
    int y;
    int width;
    int height;
} WidgetSize;

typedef struct Icon
{
    RGBA *image;
    char text[MAX_SHORT_STRLEN];
    struct Icon *next;
} Icon;

typedef struct Image
{
    struct RGB *image;
} Image;

// only command stack
enum CommandType
{
    ADD,
    DEL
};

typedef struct Command
{
    enum CommandType type;
    int index;
    char *content;
    char *old_content;
    int isBatch;
} Command;

typedef struct CommandStack
{
    struct Command stack[MAX_LONG_STRLEN];
    int stack_pos;
    int max_stack_pos;
} CommandStack;

typedef struct TextArea
{
    int begin_line;
    int current_line;
    int current_pos;
    int insert_index;

    // for select
    int select_start_index;
    int select_end_index;

    // copied select index
    int copy_start_index;
    int copy_end_index;

    // copied text
    char *temp;

    // has copied text
    int isCopying;

    struct RGBA color;
    char text[MAX_LONG_STRLEN];
    Handler onKeyDown;
    Handler onLeftClick;

    struct RGBA colors[MAX_LONG_STRLEN];

    struct CommandStack command_stack;

    char search_text[BUF_SIZE];

    char replace_text[BUF_SIZE];

    // 0 no 1 search 2 replace
    int isSearching;

    int scale;

    int isInTerminal;

    char cmd[BUF_SIZE];

    char cmd_res[BUF_SIZE];

    char filename[BUF_SIZE];

} TextArea;

typedef struct FileList
{
    int direction;
    Icon *file_list;
    int file_num;
    char path[MAX_LONG_STRLEN];
    RGBA *image[FILE_TYPE_NUM];
    Handler onDoubleClick;
    Handler onLeftClick;
    Handler onFileChange;
} FileList;

typedef union WidgetBase
{
    TextArea *textArea;
    FileList *fileList;
    Image *image;
} WidgetBase;

typedef struct Widget
{
    int type;
    WidgetSize size;
    painter paint;
    WidgetBase context;
} Widget;

typedef struct Window
{
    int width;
    int height;
    int handler;
    int widget_number;
    RGB *window_buf;
    Widget widgets[MAX_WIDGET];
} Window;

#endif
