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
#include <pthread.h>

#include "rax.h"
#include "contest.h"

#define BUF_LEN (1 << 10) * 64

typedef struct FileEntry
{
    FILE *file;
    char *name;
} FileEntry;

unsigned char c[62] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z'};
FileEntry files[62];

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

static inline void mapFile(int fd, char **data, size_t *sz)
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

static inline void cleanup(int fd, char *data, size_t sz)
{
    munmap(data, sz);
    close(fd);
}

// static void doProcess(char *data, rax *rt, raxIterator *iter)
// {
//     while (*data != 0x0)
//     {
//         int measurement;
//         char *old = data;
//         data = parse_number(&measurement, data + 129);
//         raxInsertNum(rt, old, 128, measurement);
//     }
// }

static int resultToBuf(char *buf, raxIterator *iter)
{
    int len = 0;
    while (raxNext(iter))
    {
        int n = sprintf(buf, "%.*s:%.1f/%.1f/%.1f\n", (int)iter->key_len, (char *)iter->key, (float)((Group *)iter->data)->min / 10.0,
                        (float)((Group *)iter->data)->sum / (float)((Group *)iter->data)->count / 10.0, (float)((Group *)iter->data)->max / 10.0);
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
    } while (resultToBuf(buf, iter));

    fclose(file);
}

static int getIndex(char c)
{
    int i = c - 48;
    switch (i)
    {
    case 0 ... 9:
        return i;
    case 17 ... 43:
        return i - 7;
    case 49 ... 74:
        return i - 13;
    default:
        break;
    }
}

static void *splitFile(void *file)
{
    size_t sz;
    char *data;
    int fd = openFile((char *)file);
    mapFile(fd, &data, &sz);

    while (*data != 0x0)
    {
        int measurement;
        char *old = data;
        int len;
        data = parse_number(&measurement, &len, data + 129);
        char c = old[0];
        int index = getIndex(c);
        FILE *file = files[index].file;
        fwrite(old, 128 + len + 2, 1, file);
    }
    cleanup(fd, data, sz);
    return NULL;
}

static void processData(rax *rt, char *data)
{
    while (*data != 0x0)
    {
        int measurement;
        char *old = data;
        int len;
        data = parse_number(&measurement, &len, data + 129);
        raxInsertNum(rt, old, measurement);
    }
}

static int process(char *file)
{

    rax *rt = raxNew();
    raxIterator iter;
    raxStart(&iter, rt);

    size_t sz;
    char *data;
    int fd = openFile((char *)file);
    mapFile(fd, &data, &sz);

    processData(rt, data);

    outputResult(&iter);
    raxStop(&iter);
    int numele = rt->numele;
    raxFree(rt);
    return numele;
}

void setup()
{
    mkdir("/tmp/larry", 0777);
    char base[] = "/tmp/larry/";
    char suffix[] = ".txt";
    for (int i = 0; i < 62; i++)
    {
        char name[16];
        memcpy(name, &base, 10);
        memset(name + 10, c[i], 1);
        memcpy(name + 11, &suffix, 5);
        FILE *file = fopen(name, "a");
        if (file == NULL)
        {
            perror("error opening file");
            exit(EXIT_FAILURE);
        }
        files[i].file = file;
        files[i].name = malloc(23);
        memcpy(files[i].name, &name[0], 16);
    }
}

int main(int argc, char const *argv[])
{
    setup();

    pthread_t thread[2];
    pthread_create(&thread[0], NULL, splitFile, "/app/data/data1.txt");
    pthread_create(&thread[1], NULL, splitFile, "/app/data/data2.txt");
    pthread_join(thread[0], NULL);
    pthread_join(thread[1], NULL);

     for (size_t i = 0; i < 62; i++)
    {
        fflush(files[i].file);
        fclose(files[i].file);
    }

    // for (size_t i = 0; i < 62; i++)
    // {
    //     process(files[i].name);
    // }

    return 0;
}
