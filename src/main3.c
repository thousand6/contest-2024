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

#define MAX_KEY_CAPABILITY 53
#define BUF_LEN (1 << 10) * 32
#define BLOCK_SIZE (1 << 10) * 64

typedef struct FileMapContainer
{
    int fd;
    size_t fileSize;
    char boundary[150];
    char *data;
    off_t offset;
    size_t dataLength;
    size_t mapSize;
    char *origin;
} FileMapContainer;

// 把浮点数解析成int，加快后续计算。
// ASCII表中，代表数字的字符的int值比所代表的数字本身要大，所以需要减掉相应的差值。
static inline void *parse_number(int *dest, char *s)
{
    if (s[1] == '.')
    {
        *dest = s[0] * 10 + s[2] - ('0' * 11);
    }
    if (s[2] == '.')
    {
        *dest = s[0] * 100 + s[1] * 10 + s[3] - ('0' * 111);
    }
    if (s[3] == '.')
    {
        *dest = s[0] * 1000 + s[1] * 100 + s[2] * 10 + s[4] - ('0' * 1111);
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

static inline void initBoundary(char boundary[150])
{
    memset(boundary, 0, 150);
}

static FileMapContainer *newContainer(int fd)
{
    FileMapContainer *c = malloc(sizeof(FileMapContainer));
    c->fd = fd;
    // memset(c->boundary, 0, 150);
    initBoundary(c->boundary);
    struct stat sb;
    if (fstat(fd, &sb) == -1)
    {
        perror("error getting file size");
        exit(EXIT_FAILURE);
    }
    c->fileSize = (size_t)sb.st_size;
    c->offset = 0;
    c->dataLength = 0;
}

static int mapFile(FileMapContainer *container)
{
    if (container->offset >= container->fileSize)
    {
        return 0;
    }

    size_t map_size = (container->fileSize - container->offset) < BLOCK_SIZE ? (container->fileSize - container->offset) : BLOCK_SIZE;
    char *data = mmap(NULL, map_size, PROT_READ, MAP_SHARED, container->fd, container->offset);
    container->offset += map_size;
    container->origin = data;
    container->dataLength = map_size;
    container->mapSize = map_size;
    if (container->boundary[0] != 0)
    {
        char *old = data;
        int i = 0;
        while (*(data++) != '\n')
        {
            i++;
        }
        container->dataLength -= ++i;
        int j = 0;
        while (container->boundary[j++] != 0)
        {
        }
        memcpy(container->boundary + j - 1, old, i);
    }
    container->data = data;
    return 1;
}

static inline void cleanup(int fd, char *data, size_t sz)
{
    munmap(data, sz);
    close(fd);
}

static void putTree(char *start, raxIterator *iter, char *line, char *biggest)
{
    rax *rt = iter->rt;
    int measurement;
    parse_number(&measurement, line + 129);
    if (memcmp(start, line, 128) >= 0)
    {
        return;
    }
    if (rt->numele < MAX_KEY_CAPABILITY)
    {
        // raxInsert(rt, line, 128, NULL, NULL);
        raxInsertNum(rt, line, measurement);
        if (rt->numele == MAX_KEY_CAPABILITY)
        {
            raxSeek(iter, "$", (unsigned char *)NULL, 0);
            raxPrev(iter);
            memcpy(biggest, iter->key, 128);
        }
        return;
    }

    {
        if (memcmp(biggest, line, 128) >= 0)
        {
            raxInsertNum(rt, line, measurement);
            // raxInsert(rt, line, 128, NULL, NULL);
            if (rt->numele > MAX_KEY_CAPABILITY)
            {
                raxRemove128(rt, biggest, NULL);
                raxSeek(iter, "$", (unsigned char *)NULL, 0);
                raxPrev(iter);
                memcpy(biggest, iter->key, 128);
            }
        }
    }
}

static void doProcess(char *start, raxIterator *iter, int fd)
{
    FileMapContainer *container = newContainer(fd);
    char biggest[128];
    while (1)
    {
        if (!mapFile(container))
        {
            break;
        }
        if (container->boundary[0] != 0)
        {
            putTree(start, iter, container->boundary, biggest);
            // memset(container->boundary, 0, 150);
            initBoundary(container->boundary);
        }
        int i = 0, k = 0;
        while (1)
        {
            while (k++ < container->dataLength && *(container->data + i++) != '\n')
            {
            }
            if (k <= container->dataLength)
            {
                putTree(start, iter, container->data, biggest);
                container->data += i;
                i = 0;
                continue;
            }
            memcpy(container->boundary, container->data, i);
            break;
        }
        munmap(container->origin, container->mapSize);
    }
}

static int resultToBuf(char *buf, raxIterator *iter)
{
    int len = 0;
    while (raxNext(iter))
    {
        int n = sprintf(buf, "%.*s:%.1f/%.1f/%.1f\n", 128, (char *)iter->key, (float)((Group *)iter->data)->min / 10.0,
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
    FILE *file = fopen("/mnt/d/output-larry.txt", "a");
    raxSeek(iter, "^", (unsigned char *)NULL, 0);
    char buf[BUF_LEN];
    int len = resultToBuf(buf, iter);
    do
    {
        fwrite(buf, len, 1, file);
    } while (resultToBuf(buf, iter));

    fclose(file);
}

static int process(char *start)
{
    rax *rt = raxNew();
    raxIterator iter;
    raxStart(&iter, rt);

    char *file = "/mnt/d/data1.txt";
    int fd = openFile(file);
    doProcess(start, &iter, fd);
    // cleanup(fd, data, sz);

    // file = "/app/data/data2.txt";
    // fd = openFile(file);
    // mapFile(fd, &data, &sz);
    // doProcess(start, data, rt, &iter);
    // cleanup(fd, data, sz);
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
    memset(s, 0, 128);
    int total = 0, numele;
    while (!((numele = process(s)) < MAX_KEY_CAPABILITY))
    {
        total += numele;
    }
    total += numele;
    printf("total:%d\n", total);
    return 0;
}
