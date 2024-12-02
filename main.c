/*
 * 很多思路从 https://github.com/dannyvankooten/1brc 这个项目借鉴而来
 */

#include <stdlib.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>

#include "rax.h"
#include "contest.h"

#define MAX_KEY_CAPABILITY 2

// 把浮点数解析成int，加快后续计算。
// ASCII表中，代表数字的字符的int值比所代表的数字本身要大，所以需要减掉相应的差值。
static inline char *parse_number(int *dest, char *s)
{
    if (s[1] == '.')
    {
        *dest = s[0] * 10 + s[2] - ('0' * 11);
        return s + 5;
    }
    if (s[2] == '.')
    {
        *dest = s[0] * 100 + s[1] * 10 + s[3] - ('0' * 111);
        return s + 6;
    }
    if (s[3] == '.')
    {
        *dest = s[0] * 1000 + s[1] * 100 + s[2] * 10 + s[4] - ('0' * 1111);
        return s + 7;
    }
}

int openFile(char *file)
{
    int fd = open(file, O_RDONLY);
    if (!fd)
    {
        perror("error opening file");
        exit(EXIT_FAILURE);
    }
    return fd;
}

void mapFile(int fd, char **data, size_t *sz)
{
    struct stat sb;
    if (fstat(fd, &sb) == -1)
    {
        perror("error getting file size");
        exit(EXIT_FAILURE);
    }

    // mmap entire file into memory
    *sz = (size_t)sb.st_size;
    *data = mmap(NULL, *sz, PROT_READ, MAP_SHARED, fd, 0);
    if (data == MAP_FAILED)
    {
        perror("error mmapping file");
        exit(EXIT_FAILURE);
    }
}

void cleanup(int fd, char *data, size_t sz)
{
    munmap(data, sz);
    close(fd);
}

void doProcess(char *start, char *data, rax *rt, raxIterator *iter)
{
    while (*data != 0x0)
    {
        int measurement;
        char *old = data;
        data = parse_number(&measurement, data + 129);
        char biggest[128];
        if (rt->numele < MAX_KEY_CAPABILITY)
        {
            if (memcmp(start, old, 128) < 0)
            {
                raxInsertNum(rt, old, 128, measurement);
            }
            if (rt->numele == MAX_KEY_CAPABILITY)
            {
                raxSeek(iter, "$", (unsigned char *)NULL, 0);
                raxPrev(iter);
                memcpy(biggest, iter->key, 128);
            }
        }
        else
        {
            if (memcmp(start, old, 128) < 0)
            {
                if (memcmp(biggest, old, 128) >= 0)
                {
                    raxInsertNum(rt, old, 128, measurement);
                    if (rt->numele > MAX_KEY_CAPABILITY)
                    {
                        raxRemove(rt, biggest, 128, NULL);
                        raxSeek(iter, "$", (unsigned char *)NULL, 0);
                        raxPrev(iter);
                        memcpy(biggest, iter->key, 128);
                    }
                }
            }
        }
    }
}

void outputResult(raxIterator *iter)
{
    raxSeek(iter, "^", (unsigned char *)NULL, 0);
    while (raxNext(iter))
    {
       printf("key: %.*s, mean: %.1f, max: %.1f, min: %.1f\n", (int)iter->key_len, (char*)iter->key, (float)((Group*)iter->data)->sum / (float)((Group*)iter->data)->count / 10.0, (float)((Group*)iter->data)->max / 10.0, (float)((Group*)iter->data)->min / 10.0);
    }
}

int process(char *start)
{
    rax *rt = raxNew();
    raxIterator iter;
    raxStart(&iter, rt);

    size_t sz;
    char *data;

    char *file = "/mnt/d/data1.txt";
    int fd = openFile(file);
    mapFile(fd, &data, &sz);
    doProcess(start, data, rt, &iter);
    cleanup(fd, data, sz);

    file = "/mnt/d/data1.txt";
    fd = openFile(file);
    mapFile(fd, &data, &sz);
    doProcess(start, data, rt, &iter);
    cleanup(fd, data, sz);

    outputResult(&iter);

    raxSeek(&iter, "$", (unsigned char *)NULL, 0);
    raxPrev(&iter);
    memcpy(start, iter.key, 128);
    raxStop(&iter);
    int numele = rt->numele;
    raxFree(rt);
    return numele;
}

int main(int argc, char const *argv[])
{
    char s[128];
    memset(s, '0', 128);
    while (!(process(s) < MAX_KEY_CAPABILITY))
    {
    }

    return 0;
}
