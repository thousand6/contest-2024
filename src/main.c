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

#define MAX_KEY_CAPABILITY 600000
#define BUF_LEN (1 << 10) * 64

// 把浮点数解析成int，加快后续计算。
// ASCII表中，代表数字的字符的int值比所代表的数字本身要大，所以需要减掉相应的差值。
static inline void parse_number(int *dest, char *s)
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

static inline FILE *openFile(char *file)
{
    FILE *stream = fopen(file, "r");
    if (file = NULL)
    {
        perror("error opening file");
        exit(EXIT_FAILURE);
    }
    return stream;
}

static void doProcess(char *start, FILE *file, raxIterator *iter)
{
    rax *rt = iter->rt;
    char line[136];
    while (fgets(line, 136, file) != NULL && line[0] != '\n' /*处理最后一个空行*/)
    {
        int measurement;
        parse_number(&measurement, line + 129);
        char biggest[128];
        if (memcmp(start, line, 128) >= 0)
        {
            continue;
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
        }
        else
        {
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
    }
    fclose(file);
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

    char *file = "/app/data/data1.txt";
    FILE *stream = openFile(file);
    doProcess(start, stream, &iter);

    // file = "/app/data/data2.txt";
    // stream = openFile(file);
    // doProcess(start, stream, &iter);

    // outputResult(&iter);

    raxSeek(&iter, "$", (unsigned char *)NULL, 0);
    raxPrev(&iter);
    memcpy(start, iter.key, 128);
    printf("start after is %.*s\n", 128, start);
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
