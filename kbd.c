#include "types.h"
#include "x86.h"
#include "defs.h"
#include "kbd.h"
#include "msg.h"
#include "spinlock.h"

static struct spinlock kbdlock;

int
kbdgetc(void)
{
  static uint shift;
  static uchar *charcode[4] = {
    normalmap, shiftmap, ctlmap, ctlmap
  };
  uint st, data, c;

  st = inb(KBSTATP);
  if((st & KBS_DIB) == 0)
    return -1;
  data = inb(KBDATAP);
  
  Message msg;

  if(data == 0xE0){
    shift |= E0ESC;
    return 0;
  } else if(data & 0x80){
    data = (shift & E0ESC ? data : data & 0x7F);
    shift &= ~(shiftcode[data] | E0ESC);
    c = normalmap[data];
    msg.msg_type = M_KEY_UP;
    msg.params[0] = c;
    msg.params[1] = shift;
    wmHandleMessage(&msg);
    return 0;
  } else if(shift & E0ESC){
    data |= 0x80;
    shift &= ~E0ESC;
  }

  shift |= shiftcode[data];
  shift ^= togglecode[data];
  msg.msg_type = M_KEY_DOWN;
  msg.params[0] = normalmap[data];
  msg.params[1] = shift;
  wmHandleMessage(&msg);
  
  c = charcode[shift & (CTL | SHIFT)][data];
  if(shift & CAPSLOCK){
    if('a' <= c && c <= 'z')
      c += 'A' - 'a';
    else if('A' <= c && c <= 'Z')
      c += 'a' - 'A';
  }
  
  return c;
}

void
kbdintr(void)
{
  acquire(&kbdlock);
	kbdgetc();
	release(&kbdlock);
}

