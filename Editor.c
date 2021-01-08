#include "types.h"
#include "color.h"
#include "msg.h"
#include "ui.h"
#include "user.h"
#include "fcntl.h"

int main(int argc, char *argv[])
{
    struct RGBA black = (struct RGBA){0, 0, 0, 255};
    Window editor;
    editor.width = 770;
    editor.height = 570;
    UI_createWindow(&editor, "Editor", 0);
    int file = -1;
    char filename[40];
    if (argc > 1)
    {
        file = open(argv[1], 0);
        strcpy(filename, argv[1]);
    }
    else
    {
        strcpy(filename, "Untitled.txt");
    }

    int panel = addTextAreaWidget(&editor, black, "", 0, 0, 770, 570, filename);
    if (file >= 0)
    {
        read(file, editor.widgets[panel].context.textArea->text, 512);
        close(file);
    }

    drawAllWidget(&editor);
    mainLoop(&editor);
    return 0;
}
