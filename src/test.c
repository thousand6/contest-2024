#include <stdlib.h>
#include <stdio.h>
#include "rax.h"
#include "contest.h"

unsigned char c[62] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z'};

static unsigned char *gen_rand_string()
{
    unsigned char *res = malloc(128 * sizeof(char));
    for (int i = 0; i < 12; i++)
    {
        res[i] = c[rand() % 10];
    }
    return res;
}

int main(int argc, char const *argv[])
{
    rax *rt = raxNew();
    // for (int i = 0; i < 20000000; i++)
    // {
    //     unsigned char *s = gen_rand_string();
    //     raxInsertNum(rt, s, 12, rand() % 500);
    // }
    raxInsertNum(rt, "foo", 3, 2);
    raxInsertNum(rt, "foo", 3, 4);
    raxInsertNum(rt, "fbo", 3, 2);
    raxInsertNum(rt, "fbo", 3, 7);
    raxIterator iter;
    raxStart(&iter, rt);
    raxSeek(&iter,"^",(unsigned char*)NULL,1);
    while (raxNext(&iter))
    {
        printf("key: %s, mean: %ld, max: %d, min: %d\n", (char*)iter.key, ((Group*)iter.data)->sum / ((Group*)iter.data)->count, ((Group*)iter.data)->max, ((Group*)iter.data)->min);
    }
    raxSeek(&iter,"$",(unsigned char*)NULL,1);
    while (raxPrev(&iter))
    {
        printf("key: %s, mean: %ld, max: %d, min: %d\n", (char*)iter.key, ((Group*)iter.data)->sum / ((Group*)iter.data)->count, ((Group*)iter.data)->max, ((Group*)iter.data)->min);
    }
    raxStop(&iter);
    // getchar();
    return 0;
}
