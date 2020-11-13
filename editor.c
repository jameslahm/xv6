#include "types.h"
#include "stat.h"
#include "user.h"
#include "fcntl.h"
#include "fs.h"

#define NONE "\e[0m"
#define BLACK "\e[0;30m"
#define L_BLACK "\e[1;30m"
#define RED "\e[0;31m"
#define L_RED "\e[1;31m"
#define GREEN "\e[0;32m"
#define L_GREEN "\e[1;32m"
#define YELLOW "\e[0;33m"
#define L_YELLOW "\e[1;33m"
#define BLUE "\e[0;34m"
#define L_BLUE "\e[1;34m"
#define PURPLE "\e[0;35m"
#define L_PURPLE "\e[1;35m"
#define CYAN "\e[0;36m"
#define L_CYAN "\e[1;36m"
#define GRAY "\e[0;37m"
#define WHITE "\e[1;37m"

#define BUF_SIZE 256
#define MAX_LINE_NUMBER 256

#define NULL 0

// show text, flag control if highlight
void show_text(char *text[], int flag)
{
    // no highlight
    if (!flag)
    {
        for (int i = 0; i < MAX_LINE_NUMBER && text[i] != NULL; i++)
        {
            printf(1, "\e[1;30m%d%d%d\e[0;32m| \e[0m%s\n", (i + 1) / 100, (i + 1) % 100 / 10, (i + 1) % 10, text[i]);
        }
    }

    // highlight
    else
    {
        for (int i = 0; i < MAX_LINE_NUMBER && text[i] != NULL; i++)
        {
            printf(1, "\e[1;30m%d%d%d\e[0;32m| \e[0m", (i + 1) / 100, (i + 1) % 100 / 10, (i + 1) % 10);

            int pos = 0;
            for (; text[i][pos] != ' ' && text[i][pos] != '\0'; pos++)
                ;

            int base = 0;
            int len = strlen(text[i]);

            // annotation
            if (text[i][0] == '/' && text[i][1] == '/')
            {
                printf(1, "\e[1;30m%s\n\e[0m", text[i]);
                continue;
            }

            while (text[i][base] != '\0')
            {

                // numbers
                if (text[i][base] >= '0' && text[i][base] <= '9')
                {
                    printf(1, "\e[1;36m%c\e[0m", text[i][base]);
                    base += 1;
                }

                // string
                // TODO: fix possible segement fault
                else if (text[i][base] == '"')
                {
                    printf(1, "%c", text[i][base]);
                    int end = base + 1;
                    for (; text[i][end] != '"' && text[i][end] != '\0'; end++)
                        ;
                    for (int j = base + 1; j < end; j++)
                    {
                        printf(1, "\e[1;31m%c\e[0m", text[i][j]);
                    }
                    printf(1, "%c", text[i][end]);
                    base = end + 1;
                }

                // single char
                else if (text[i][base] == '\'')
                {
                    printf(1, "%c", text[i][base]);
                    if (base + 1 < len)
                        printf(1, "%c", text[i][base + 1]);
                    if (base + 2 < len)
                        printf(1, "%c", text[i][base + 2]);
                    base += 3;
                }

                // int
                else if ((len - base) >= 3 && text[i][base] == 'i' && text[i][base + 1] == 'n' && text[i][base + 2] == 't')
                {
                    printf(1, "\e[1;32m%c\e[0m", text[i][base]);
                    printf(1, "\e[1;32m%c\e[0m", text[i][base + 1]);
                    printf(1, "\e[1;32m%c\e[0m", text[i][base + 2]);
                    base += 3;
                }

                // long
                else if ((len - base) >= 4 && text[i][base] == 'l' && text[i][base + 1] == 'o' && text[i][base + 2] == 'n' && text[i][base + 3] == 'g')
                {
                    printf(1, "\e[1;32m%c\e[0m", text[i][base]);
                    printf(1, "\e[1;32m%c\e[0m", text[i][base + 1]);
                    printf(1, "\e[1;32m%c\e[0m", text[i][base + 2]);
                    printf(1, "\e[1;32m%c\e[0m", text[i][base + 3]);
                    base += 4;
                }

                // double
                else if ((len - base) >= 6 && text[i][base] == 'd' && text[i][base + 1] == 'o' && text[i][base + 2] == 'u' && text[i][base + 3] == 'b' && text[i][base + 4] == 'l' && text[i][base + 5] == 'e')
                {
                    printf(1, "\e[1;32m%c\e[0m", text[i][base]);
                    printf(1, "\e[1;32m%c\e[0m", text[i][base + 1]);
                    printf(1, "\e[1;32m%c\e[0m", text[i][base + 2]);
                    printf(1, "\e[1;32m%c\e[0m", text[i][base + 3]);
                    printf(1, "\e[1;32m%c\e[0m", text[i][base + 4]);
                    printf(1, "\e[1;32m%c\e[0m", text[i][base + 5]);
                    base += 6;
                }

                // float
                else if ((len - base) >= 5 && text[i][base] == 'f' && text[i][base + 1] == 'l' && text[i][base + 2] == 'o' && text[i][base + 3] == 'a' && text[i][base + 4] == 't')
                {
                    printf(1, "\e[1;32m%c\e[0m", text[i][base]);
                    printf(1, "\e[1;32m%c\e[0m", text[i][base + 1]);
                    printf(1, "\e[1;32m%c\e[0m", text[i][base + 2]);
                    printf(1, "\e[1;32m%c\e[0m", text[i][base + 3]);
                    printf(1, "\e[1;32m%c\e[0m", text[i][base + 4]);
                    base += 5;
                }

                // char
                else if ((len - base) >= 4 && text[i][base] == 'c' && text[i][base + 1] == 'h' && text[i][base + 2] == 'a' && text[i][base + 3] == 'r')
                {
                    printf(1, "\e[1;32m%c\e[0m", text[i][base]);
                    printf(1, "\e[1;32m%c\e[0m", text[i][base + 1]);
                    printf(1, "\e[1;32m%c\e[0m", text[i][base + 2]);
                    printf(1, "\e[1;32m%c\e[0m", text[i][base + 3]);
                    base += 4;
                }

                // if
                else if ((len - base) >= 2 && text[i][base] == 'i' && text[i][base + 1] == 'f')
                {
                    printf(1, "\e[1;33m%c\e[0m", text[i][base]);
                    printf(1, "\e[1;33m%c\e[0m", text[i][base + 1]);
                    base += 2;
                }

                // else
                else if ((len - base) >= 4 && text[i][base] == 'e' && text[i][base + 1] == 'l' && text[i][base + 2] == 's' && text[i][base + 3] == 'e')
                {
                    printf(1, "\e[1;33m%c\e[0m", text[i][base]);
                    printf(1, "\e[1;33m%c\e[0m", text[i][base + 1]);
                    printf(1, "\e[1;33m%c\e[0m", text[i][base + 2]);
                    printf(1, "\e[1;33m%c\e[0m", text[i][base + 3]);
                    base += 4;
                }

                // else if
                else if ((len - base) >= 7 && text[i][base] == 'e' && text[i][base + 1] == 'l' && text[i][base + 2] == 's' && text[i][base + 3] == 'e' && text[i][base + 4] == ' ' && text[i][base + 5] == 'i' && text[i][base + 6] == 'f')
                {
                    printf(1, "\e[1;33m%c\e[0m", text[i][base]);
                    printf(1, "\e[1;33m%c\e[0m", text[i][base + 1]);
                    printf(1, "\e[1;33m%c\e[0m", text[i][base + 2]);
                    printf(1, "\e[1;33m%c\e[0m", text[i][base + 3]);
                    printf(1, "%c", text[i][base + 4]);
                    printf(1, "\e[1;33m%c\e[0m", text[i][base + 5]);
                    printf(1, "\e[1;33m%c\e[0m", text[i][base + 6]);
                    base += 7;
                }

                // for
                else if ((len - base) >= 3 && text[i][base] == 'f' && text[i][base + 1] == 'o' && text[i][base + 2] == 'r')
                {
                    printf(1, "\e[1;34m%c\e[0m", text[i][base]);
                    printf(1, "\e[1;34m%c\e[0m", text[i][base + 1]);
                    printf(1, "\e[1;34m%c\e[0m", text[i][base + 2]);
                    base += 3;
                }

                // while
                else if ((len - base) >= 5 && text[i][base] == 'w' && text[i][base + 1] == 'h' && text[i][base + 2] == 'i' && text[i][base + 3] == 'l' && text[i][base + 4] == 'e')
                {
                    printf(1, "\e[1;34m%c\e[0m", text[i][base]);
                    printf(1, "\e[1;34m%c\e[0m", text[i][base + 1]);
                    printf(1, "\e[1;34m%c\e[0m", text[i][base + 2]);
                    printf(1, "\e[1;34m%c\e[0m", text[i][base + 3]);
                    printf(1, "\e[1;34m%c\e[0m", text[i][base + 4]);
                    base += 5;
                }

                // static
                else if ((len - base) >= 6 && text[i][base] == 's' && text[i][base + 1] == 't' && text[i][base + 2] == 'a' && text[i][base + 3] == 't' && text[i][base + 4] == 'i' && text[i][base + 5] == 'c')
                {
                    printf(1, "\e[1;33m%c\e[0m", text[i][base]);
                    printf(1, "\e[1;33m%c\e[0m", text[i][base + 1]);
                    printf(1, "\e[1;33m%c\e[0m", text[i][base + 2]);
                    printf(1, "\e[1;33m%c\e[0m", text[i][base + 3]);
                    printf(1, "\e[1;33m%c\e[0m", text[i][base + 4]);
                    base += 6;
                }

                // const
                else if ((len - base) >= 5 && text[i][base] == 'c' && text[i][base + 1] == 'o' && text[i][base + 2] == 'n' && text[i][base + 3] == 's' && text[i][base + 4] == 't')
                {
                    printf(1, "\e[1;33m%c\e[0m", text[i][base]);
                    printf(1, "\e[1;33m%c\e[0m", text[i][base + 1]);
                    printf(1, "\e[1;33m%c\e[0m", text[i][base + 2]);
                    printf(1, "\e[1;33m%c\e[0m", text[i][base + 3]);
                    printf(1, "\e[1;33m%c\e[0m", text[i][base + 4]);
                    base += 5;
                }

                else if ((len - base) >= 8 && text[i][base] == 'c' && text[i][base + 1] == 'o' && text[i][base + 2] == 'n' && text[i][base + 3] == 't' && text[i][base + 4] == 'i' && text[i][base + 5] == 'n' && text[i][base + 6] == 'u' && text[i][base + 7] == 'e')
                {
                    printf(1, "\e[1;35m%c\e[0m", text[i][base]);
                    printf(1, "\e[1;35m%c\e[0m", text[i][base + 1]);
                    printf(1, "\e[1;35m%c\e[0m", text[i][base + 2]);
                    printf(1, "\e[1;35m%c\e[0m", text[i][base + 3]);
                    printf(1, "\e[1;35m%c\e[0m", text[i][base + 4]);
                    printf(1, "\e[1;35m%c\e[0m", text[i][base + 5]);
                    printf(1, "\e[1;35m%c\e[0m", text[i][base + 6]);
                    printf(1, "\e[1;35m%c\e[0m", text[i][base + 7]);
                    base += 8;
                }

                else if ((len - base) >= 6 && text[i][base] == 'r' && text[i][base + 1] == 'e' && text[i][base + 2] == 't' && text[i][base + 3] == 'u' && text[i][base + 4] == 'r' && text[i][base + 5] == 'n')
                {
                    printf(1, "\e[1;35m%c\e[0m", text[i][base]);
                    printf(1, "\e[1;35m%c\e[0m", text[i][base + 1]);
                    printf(1, "\e[1;35m%c\e[0m", text[i][base + 2]);
                    printf(1, "\e[1;35m%c\e[0m", text[i][base + 3]);
                    printf(1, "\e[1;35m%c\e[0m", text[i][base + 4]);
                    printf(1, "\e[1;35m%c\e[0m", text[i][base + 5]);
                    base += 6;
                }

                else if ((len - base) >= 5 && text[i][base] == 'b' && text[i][base + 1] == 'r' && text[i][base + 2] == 'e' && text[i][base + 3] == 'a' && text[i][base + 4] == 'k')
                {
                    printf(1, "\e[1;35m%c\e[0m", text[i][base]);
                    printf(1, "\e[1;35m%c\e[0m", text[i][base + 1]);
                    printf(1, "\e[1;35m%c\e[0m", text[i][base + 2]);
                    printf(1, "\e[1;35m%c\e[0m", text[i][base + 3]);
                    printf(1, "\e[1;35m%c\e[0m", text[i][base + 4]);
                    base += 5;
                }

                else
                {
                    printf(1, "%c", text[i][base]);
                    base += 1;
                }
            }
            printf(1, "\n");
        }
    }
}

int get_line_number(char *text[])
{
    int line_number = 0;
    for (int i = 0; i < MAX_LINE_NUMBER; i++)
    {
        if (text[i] != NULL)
        {
            line_number++;
        }
    }
    return line_number;
}

void handle_ins(char *text[], int line_number, char *content)
{
    int current_line_number = get_line_number(text);

    if (line_number > current_line_number)
    {
        printf(2, "\e[0;31mInvalid line number\n\e[0m");
        return;
    }

    if (*content == '\0')
    {
        printf(1, "\e[0;35mPlease input the content: \e[0m");
        char input[BUF_SIZE] = {0};
        gets(input, BUF_SIZE - 1);
        input[strlen(input) - 1] = '\0';
        content = input;
    }

    text[current_line_number] = (char *)malloc(BUF_SIZE);
    for (int i = current_line_number; i > line_number; i--)
    {
        strcpy(text[i], text[i - 1]);
    }
    memset(text[line_number], 0, BUF_SIZE);
    strcpy(text[line_number], content);
    return;
}

void handle_mod(char *text[], int line_number, char *content)
{
    int current_line_number = get_line_number(text);

    if (line_number >= current_line_number)
    {
        printf(2, "\e[0;31mInvalid line number\n\e[0m");
        return;
    }

    if (*content == '\0')
    {
        printf(1, "\e[0;35mPlease input the content: \e[0m");
        char input[BUF_SIZE] = {0};
        gets(input, BUF_SIZE - 1);
        input[strlen(input) - 1] = '\0';
        content = input;
    }

    memset(text[line_number], 0, BUF_SIZE);
    strcpy(text[line_number], content);
    return;
}

void handle_del(char *text[], int line_number)
{
    int current_line_number = get_line_number(text);

    if (line_number >= current_line_number)
    {
        printf(2, "\e[0;31mInvalid line number\n\e[0m");
        return;
    }

    for (int i = line_number; i < current_line_number - 1; i++)
    {
        strcpy(text[i], text[i + 1]);
    }
    free(text[current_line_number - 1]);
    text[current_line_number - 1] = NULL;
    return;
}

void handle_save(char *filename, char *text[])
{
    // delete file
    unlink(filename);
    int fd = open(filename, O_CREATE | O_WRONLY);
    if (fd == -1)
    {
        printf(2, "\e[0;31mSave failed\n\e[0m");
        return;
    }
    for (int i = 0; i < MAX_LINE_NUMBER && text[i] != NULL; i++)
    {
        write(fd, text[i], BUF_SIZE);
        if (text[i + 1] != NULL)
        {
            printf(fd, "\n");
        }
    }
    printf(1, "\e[0;32mSave successful\n\e[0m");
    return;
}

void handle_exit()
{
}

void handle_help()
{
    printf(1, ">>> \e[1;33mInstructions for use:\n\e[0m");
    printf(1, "--------+--------------------------------------------------------------\n");
    printf(1, "\e[1;32mins-n:\e[0m 	| insert a line after line n\n");
    printf(1, "\e[1;32mmod-n:\e[0m 	| modify line n\n");
    printf(1, "\e[1;32mdel-n:\e[0m 	| delete line n\n");
    printf(1, "\e[1;32mins:\e[0m 	| insert a line after the last line\n");
    printf(1, "\e[1;32mmod:\e[0m 	| modify the last line\n");
    printf(1, "\e[1;32mdel:\e[0m 	| delete the last line\n");
    printf(1, "\e[1;32mshow:\e[0m 	| enable show current contents after executing a command.\n");
    printf(1, "\e[1;32mhide:\e[0m 	| disable show current contents after executing a command.\n");
    printf(1, "\e[1;32msave:\e[0m 	| save the file\n");
    printf(1, "\e[1;32mexit:\e[0m 	| exit editor\n");
    printf(1, "\e[1;32mhelp:\e[0m	| help info\n");
    printf(1, "\e[1;32mdisp:\e[0m	| display with highlighting\n");
    printf(1, "\e[1;32mrb:\e[0m	| rollback the file\n");
    printf(1, "--------+--------------------------------------------------------------\n");
}

// string to number
int s2num(char *src)
{
    int number = 0;
    int i = 0;
    int pos = strlen(src);
    for (; i < pos; i++)
    {
        if (src[i] == ' ')
            break;
        if (src[i] > 57 || src[i] < 48)
            return -1;
        number = 10 * number + (src[i] - 48);
    }
    return number;
}

int main(int argc, char *argv[])
{
    // check file arg
    if (argc == 1)
    {
        printf(2, "\e[0;31mPlease input the file argument!\n\e[0m");
        exit();
    }

    // open the file or create
    int fd = open(argv[1], O_RDONLY);
    if (fd == -1)
    {
        printf(2, "\e[0;31mFile %s not exist, do you want to create it? (y/n) \e[0m", argv[1]);
        char input[BUF_SIZE] = {0};
        gets(input, BUF_SIZE);
        input[strlen(input) - 1] = '\0';
        if (strcmp(input, "y") == 0)
        {
            fd = open(argv[1], O_RDONLY | O_CREATE);
            if (fd == -1)
            {
                printf(2, "\e[0;31mCreate %s failed...\n\e[0m", argv[1]);
            }
            else
            {
                printf(1, "\e[0;32mCreate %s successful!\n\e[0m", argv[1]);
            }
        }
        else
        {
            exit();
        }
    }

    // read file into text buf
    char *filename = argv[1];

    char *text[MAX_LINE_NUMBER] = {0};
    // total lines
    int line_number = 0;
    // current line offset
    int text_base = 0;

    int len;

    char buf[BUF_SIZE] = {0};
    while ((len = read(fd, buf, BUF_SIZE - 1)) > 0)
    {
        int i = 0;
        int buf_base = 0;
        for (; i < len; i++)
        {
            // newline
            if (buf[i] == '\n')
            {
                buf[i] = '\0';
                if (text[line_number] == NULL)
                {
                    text[line_number] = (char *)malloc(BUF_SIZE);
                    memset(text[line_number], 0, BUF_SIZE);
                }
                strcpy(text[line_number] + text_base, buf + buf_base);
                text_base = 0;
                buf_base = i + 1;
                line_number++;
                i++;
            }
            // buf end
            if (buf[i] == '\0')
            {
                if (text[line_number] == NULL)
                {
                    text[line_number] = (char *)malloc(BUF_SIZE);
                    memset(text[line_number], 0, BUF_SIZE);
                }
                strcpy(text[line_number] + text_base, buf + buf_base);
                text_base += i - buf_base;
                buf_base = i + 1;
                i++;
            }
        };
        memset(buf, 0, BUF_SIZE);
    }

    line_number = get_line_number(text);

    show_text(text, 1);
    while (1)
    {
        char buf[BUF_SIZE] = {0};
        // input command
        printf(1, "\e[0;34m>> \e[0m");
        gets(buf, BUF_SIZE - 1);
        buf[strlen(buf) - 1] = '\0';

        // find blank
        int pos = 0;
        for (; buf[pos] != ' ' && pos < BUF_SIZE - 1; pos++)
            ;

        if (buf[0] == 'i' && buf[1] == 'n' && buf[2] == 's')
        {
            if (buf[3] == '-')
            {
                int number = s2num(buf + 4);
                if (number != -1)
                {
                    handle_ins(text, number - 1, buf + pos + 1);
                }
                else
                {
                    printf(2, "\e[0;31minvalid command\e[0m\n");
                    continue;
                }
            }
            else
            {
                handle_ins(text, line_number, buf + pos + 1);
            }
            line_number = get_line_number(text);
            show_text(text, 1);
        }
        else if (buf[0] == 'm' && buf[1] == 'o' && buf[2] == 'd')
        {
            if (buf[3] == '-')
            {
                int number = s2num(buf + 4);
                if (number != -1)
                {
                    handle_mod(text, number - 1, buf + pos + 1);
                }
                else
                {
                    printf(2, "\e[0;31minvalid command\e[0m\n");
                    continue;
                }
            }
            else
            {
                handle_mod(text, line_number - 1, buf + pos + 1);
            }
            line_number = get_line_number(text);
            show_text(text, 1);
        }
        else if (buf[0] == 'd' && buf[1] == 'e' && buf[2] == 'l')
        {
            if (buf[3] == '-')
            {
                int number = s2num(buf + 4);
                if (number != -1)
                {
                    handle_del(text, number - 1);
                }
                else
                {
                    printf(2, "\e[0;31minvalid command\e[0m\n");
                    continue;
                }
            }
            else
            {
                handle_del(text, line_number - 1);
            }
            line_number = get_line_number(text);
            show_text(text, 1);
        }
        else if (strcmp(buf, "save") == 0)
        {
            handle_save(filename, text);
        }
        else if (strcmp(buf, "exit") == 0)
        {
            handle_exit();
            break;
        }
        else if (strcmp(buf, "help") == 0)
        {
            handle_help();
        }
        else
        {
            printf(2, "\e[0;31mminvalid command\e[0m\n");
        }
    }

    exit();
}