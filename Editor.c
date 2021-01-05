#include "types.h"
#include "color.h"
#include "msg.h"
#include "ui.h"
#include "user.h"
#include "fcntl.h"

char filename[40];

void saveBackBtn(window *win, int index, message* msg) {
    printf(1, "saving\n");
    int file = open(filename, 1 | O_CREATE);
    int i;
    for (i = 0; i < win->widget_number; i++) {
        if (win->widgets[i].type == TEXT_AREA) {
            break;
        }
    }
    int res=write(file, win->widgets[i].context.textArea->text, 512);
    close(file);
    printf(1, "saved Res: %d\n",res);
}

int main(int argc, char *argv[]) {
    RGBA black;
    black.A = 255;
    black.R = 0;
    black.G = 0;
    black.B = 0;
    // RGBA red;
    // red.A = 255;
    // red.R = 211;
    // red.G = 80;
    // red.B = 80;
    window editor;
    editor.width = 770;
    editor.height = 570;
    UI_createWindow(&editor, "Editor", 0);
    int file = -1;
    if (argc > 1) {
        file = open(argv[1], 0);
        strcpy(filename, argv[1]);
    } else {
        strcpy(filename, "Untitled.txt");
    }

    int panel = addTextAreaWidget(&editor, black, "", 0, 0, 770, 570);
    // addButtonWidget(&editor, black, red, "save", saveBackBtn, 720, 540, 80, 50);
    if (file >= 0) {
        read(file, editor.widgets[panel].context.textArea->text, 512);
        close(file);
    }

    drawAllWidget(&editor);
    mainLoop(&editor);
    return 0;
}
