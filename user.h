#ifndef __USER_H__
#define __USER_H__

struct stat;
struct rtcdate;
struct RGBA;
struct RGB;
struct Message;
struct Window;

typedef void(*Handler)(struct Window *win, int index, struct Message *msg);

// system calls
int fork(void);
int exit(void) __attribute__((noreturn));
int wait(void);
int pipe(int*);
int write(int, void*, int);
int read(int, void*, int);
int close(int);
int kill(int);
int exec(char*, char**);
int open(char*, int);
int mknod(char*, short, short);
int unlink(char*);
int fstat(int fd, struct stat*);
int link(char*, char*);
int mkdir(char*);
int chdir(char*);
int dup(int);
int getpid(void);
char* sbrk(int);
int sleep(int);
int uptime(void);

// ulib.c
int stat(char*, struct stat*);
char* strcpy(char*, char*);
void *memmove(void*, void*, int);
char* strchr(const char*, char c);
int strcmp(const char*, const char*);
void printf(int, char*, ...);
char* gets(char*, int max);
uint strlen(char*);
void* memset(void*, int, uint);
void* malloc(uint);
void free(void*);
int atoi(const char*);

// bitmap.c
int readBitmapFile(char *, struct RGBA *, int *, int *);
int read24BitmapFile(char *, struct RGB *, int *, int *);

// wm.c
int createwindow(int, int, const char *, struct RGB *, int);
int destroywindow(int);
int getmessage(int, struct Message *);
int updatewindow(int, int, int, int, int);

// ui.c
void UI_createWindow(struct Window *, const char*, int);
void UI_destroyWindow(struct Window *);
void updatePartWindow(struct Window *, int, int, int, int);
int addImageWidget(struct Window *, struct RGB *, int, int, int, int);
int addTextAreaWidget(struct Window *, struct RGBA, char *, int, int, int, int,char*);
int addFileListWidget(struct Window *, char *, int, int, int, int, int);
void drawAllWidget(struct Window *);
void mainLoop(struct Window *);

#endif
