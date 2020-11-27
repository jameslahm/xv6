#include "mouse.h"

static struct spinlock mouselock;
static int count = 0;
static int recovery = 0;
static int lastbtn,lastdowntick,lastclicktick;

static struct {
    int x_sgn,y_sgn,x_mov,y_mov;
    int l_btn,r_btn,m_btn;
    int x_overflow,y_overflow;
    uint tick;
} packet;

enum MOUSE_WAIT_COMMAND
{
    READ,
    WRITE
};

#define MOUSE_DATA_PORT 0x60
#define MOUSE_STATUS_PORT 0x64

void mouse_wait(enum MOUSE_WAIT_COMMAND type)
{
    uint timeout = 100000;
    if (type == READ)
    {
        while (timeout--)
        {
            // 可读状态
            if ((inb(MOUSE_STATUS_PORT) & 1) == 1)
            {
                return;
            }
        }
    }
    else
    {
        while(timeout--){
            // 可写状态
            if((inb(MOUSE_STATUS_PORT) & 2)==0){
                return;
            }
        }
    }
    return;
}

// read buffer
uint mouse_read(){
    mouse_wait(READ);
    return inb(0x60);
}

// wriye buffer
void mouse_write(w){
    mouse_wait(WRITE);
    outb(MOUSE_STATUS_PORT,0xd4);
    mouse_wait(WRITE);
    outb(MOUSE_DATA_PORT,w);
}


void mouseinit()
{
    uchar confbyte;

    // 启用鼠标接口
    mouse_wait(WRITE);
    outb(MOUSE_STATUS_PORT,0xa8); 

    // 启用鼠标中断
    mouse_wait(WRITE);
    // 读取配置字节
    outb(MOUSE_STATUS_PORT,0x20);
    mouse_wait(READ);
    confbyte = (inb(MOUSE_DATA_PORT) | 2);
    mouse_wait(WRITE);
    outb(MOUSE_STATUS_PORT,0x60);
    mouse_wait(WRITE);
    outb(MOUSE_DATA_PORT,confbyte);

    // 设置鼠标为默认设置
    mouse_write(0xf6);
    mouse_read();

    // 设置鼠标采样率
    mouse_write(0xf3);
    mouse_read();
    mouse_write(10);
    mouse_read();

    mouse_write(0xf4);
    mouse_read();

    initlock(&mouselock,"mouse");
    picenable(IRQ_MOUSE);
    ioapicenable(IRQ_MOUSE,0);

    count = 0;
    
    lastclicktick = lastdowntick = -1000;

}

void gen_mouse_msg(){
    // 鼠标位置溢出
    if(packet.x_overflow || packet.y_overflow) {
        return;
    }
    int x = packet.x_sgn ? (0xffffff00 | (packet.x_mov & 0xff)) : (packet.x_mov & 0xff);
	int y = packet.y_sgn ? (0xffffff00 | (packet.y_mov & 0xff)) : (packet.y_mov & 0xff);

    packet.x_mov = x;
    packet.y_mov = y;

    int btns = packet.l_btn | (packet.r_btn <<1) |(packet.m_btn<<2);
    Message msg;
    if(packet.x_mov || packet.y_mov){
        msg.msg_type = MSG_MOUSE_MOVE;
        msg.params[0] = packet.x_mov;
        msg.params[1] = packet.y_mov;
        msg.params[2] = btns;
        lastdowntick = lastclicktick = -1000;
    }
    else if(btns){
        msg.msg_type = MSG_MOUSE_DOWN;
        msg.params[0] = btns;
        lastdowntick = packet.tick;
    }
    else if(packet.tick - lastdowntick < 30){
        if(lastbtn & 1){
            msg.msg_type = MSG_MOUSE_LEFT_CLICK;
        }
        else{
            msg.msg_type = MSG_MOUSE_RIGHT_CLICK;
        }

        if(packet.tick - lastclicktick < 60){
            msg.msg_type = MSG_MOUSE_DBCLICK;
            lastclicktick = -1000;
        }
        else{
            lastclicktick = packet.tick;
        }
    }
    else{
        msg.msg_type = MSG_MOUSE_UP;
        msg.params[0]= btns;
    }
    lastbtn = btns;
    handle_message(&msg);
}

// 处理鼠标中断
void mouseintr(uint tick)
{
    acquire(&mouselock);
    int status;
    while((status = inb(MOUSE_STATUS_PORT) & 1)==1){
        int data = inb(0x60);
        count++;
        
        if(recovery == 0 && (data & 255)==0){
            recovery = 1;
        }
        else if(recovery == 1 && (data & 255)==0){
            recovery = 2;
        }
        else if((data & 255)==12){
            recovery = 0;
        }
        else
        {
            recovery = -1;
        }
        
        switch(count){
            case 1:{
                if(data & 0x08){
                    packet.y_overflow = (data >> 7) & 0x1;
                    packet.x_overflow = (data >> 6) & 0x1;
                    packet.y_sgn = (data >> 5) & 0x1;
                    packet.x_sgn = (data >> 4) & 0x1;
                    packet.m_btn = (data >> 2) & 0x1;
                    packet.r_btn = (data >> 1) & 0x1;
                    packet.l_btn = (data >> 0) & 0x1;
                }
                else{
                    count = 0;
                }
                break;
            }
            case 2:{
                packet.x_mov =data;
                break;
            }
            case 3:{
                packet.y_mov = data;
                packet.tick = tick;
                break;
            }
            default:{
                count = 0;
                break;
            }
        }

        if(recovery == 2){
            count = 0;
            recovery = -1;
        }
        else if(count == 3){
            count =0;
            gen_mouse_msg();
        }
    }
    release(&mouselock);
}