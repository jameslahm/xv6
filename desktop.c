#include "types.h"
#include "color.h"
#include "msg.h"
#include "ui.h"
#include "user.h"
#include "fcntl.h"

RGB image[800 * 600];

int main() {
    int h, w;
    Window desktop;
    desktop.width = MAX_WIDTH;
    desktop.height = MAX_HEIGHT;
    UI_createWindow(&desktop, "", 0);
    read24BitmapFile("desktop.bmp", image, &h, &w);
    addImageWidget(&desktop, image, 0, 0, MAX_WIDTH, MAX_HEIGHT);
    addFileListWidget(&desktop, "/", 1, 0, 0, MAX_WIDTH, MAX_HEIGHT - 25);
    drawAllWidget(&desktop);
    mainLoop(&desktop);
}
