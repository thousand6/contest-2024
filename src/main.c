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
#include <math.h>

#include "rax.h"
#include "contest.h"

#define MAX_KEY_CAPABILITY 60
#define BUF_LEN (1 << 10) * 64
#define CHUNK_SIZE (1 << 10) * 16

#define PAGE_SIZE sysconf(_SC_PAGE_SIZE)

// 把浮点数解析成int，加快后续计算。
// ASCII表中，代表数字的字符的int值比所代表的数字本身要大，所以需要减掉相应的差值。
static inline char *parse_number(int *dest, int *len, char *s)
{
    if (s[1] == '.')
    {
        *dest = s[0] * 10 + s[2] - ('0' * 11);
        *len = 3;
        return s + 4;
    }
    if (s[2] == '.')
    {
        *dest = s[0] * 100 + s[1] * 10 + s[3] - ('0' * 111);
        *len = 4;
        return s + 5;
    }
    if (s[3] == '.')
    {
        *dest = s[0] * 1000 + s[1] * 100 + s[2] * 10 + s[4] - ('0' * 1111);
        *len = 5;
        return s + 6;
    }
}

static inline int openFile(char *file)
{
    int fd = open(file, O_RDONLY);
    if (!fd)
    {
        perror("error opening file");
        exit(EXIT_FAILURE);
    }
    return fd;
}

static size_t doProcess(char *start, char *data, raxIterator *iter)
{
    rax *rt = iter->rt;
    size_t offset = 0;
    while (*data != 0x0)
    {
        int measurement;
        char *old = data;
        int len;
        data = parse_number(&measurement, &len, data + 129);
        char biggest[128];
        if (memcmp(start, old, 128) >= 0)
        {
            continue;
        }
        if (rt->numele < MAX_KEY_CAPABILITY)
        {
            raxInsertNum(rt, old, 128, measurement);
            if (rt->numele == MAX_KEY_CAPABILITY)
            {
                raxSeek(iter, "$", (unsigned char *)NULL, 0);
                raxPrev(iter);
                memcpy(biggest, iter->key, 128);
            }
        }
        else
        {
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
        offset += len;
        offset += 130;
        if ((offset + PAGE_SIZE) > CHUNK_SIZE)
        {
            break;
        }
    }
    return offset;
}

static inline void mapFile(char *start, int fd, raxIterator *iter)
{
    struct stat sb;
    if (fstat(fd, &sb) == -1)
    {
        perror("error getting file size");
        exit(EXIT_FAILURE);
    }
    size_t size = (size_t)sb.st_size;
    size_t offset = 0;
    while ((offset + CHUNK_SIZE) < size)
    {
        char *data = mmap(NULL, CHUNK_SIZE, PROT_READ, MAP_SHARED, fd, offset);
        if (data == MAP_FAILED)
        {
            perror("error mmapping file");
            exit(EXIT_FAILURE);
        }
        offset += doProcess(start, data, iter);
        munmap(data, CHUNK_SIZE);
    }
    close(fd);
}

static inline void cleanup(int fd, char *data, size_t sz)
{
    munmap(data, sz);
    close(fd);
}



static int resultToBuf(char *buf, raxIterator *iter)
{
    int len = 0;
    while (raxNext(iter))
    {
        int n = sprintf(buf, "%.*s:%.1f/%.1f/%.1f\n", (int)iter->key_len, (char *)iter->key, (float)((Group *)iter->data)->min / 10.0,
                        round((float)((Group *)iter->data)->sum / (float)((Group *)iter->data)->count) / 10.0, (float)((Group *)iter->data)->max / 10.0);
        len += n;
        buf += n;
        if (len + 200 >= BUF_LEN)
        {
            return len;
        }
    }
    return len;
}

static void outputResult(raxIterator *iter)
{
    FILE *file = fopen("/app/data/output-larry.txt", "a");
    if (file == NULL)
    {
        perror("error opening file");
        exit(EXIT_FAILURE);
    }
    raxSeek(iter, "^", (unsigned char *)NULL, 0);
    char buf[BUF_LEN];
    int len = resultToBuf(buf, iter);
    do
    {
        fwrite(buf, len, 1, file);
    } while ((len = resultToBuf(buf, iter)));
    fflush(file);
    fclose(file);
}

static int process(char *start)
{
    rax *rt = raxNew();
    raxIterator iter;
    raxStart(&iter, rt);

    size_t sz;
    char *data;

    char *file = "/mnt/d/data1.txt";
    int fd = openFile(file);
    mapFile(start, fd, &iter);

    // file = "/app/data/data2.txt";
    // fd = openFile(file);
    // mapFile(start, fd, &iter);

    // outputResult(&iter);

    raxSeek(&iter, "$", (unsigned char *)NULL, 0);
    raxPrev(&iter);
    memcpy(start, iter.key, 128);
    printf("start is %s\n", start);
    raxStop(&iter);
    int numele = rt->numele;
    raxFree(rt);
    return numele;
}

int main(int argc, char const *argv[])
{
    char s[128];
    memset(s, 0, 128);
    int total, numele;
    while (!((numele = process(s)) < MAX_KEY_CAPABILITY))
    {
        total += numele;
    }
    total += numele;
    printf("total:%d\n", total);

    return 0;
}
