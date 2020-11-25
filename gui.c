#include "gui.h"
#include "types.h"
#include "memlayout.h"
#include "user.h"

void initGUI() {
    // bugs leaved
    SCREEN_WIDTH = *((ushort *) (KERNBASE + 0x1012));
    SCREEN_HEIGHT = *((ushort *) (KERNBASE + 0x1014));
}