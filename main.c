#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>

#include "rax.h"
#include "contest.h"

#define MAX_KEY_CAPABILITY 400000

// 把浮点数解析成int，加快后续计算。
// ASCII表中，代表数字的字符的int值比所代表的数字本身要大，所以需要减掉相应的差值。
static inline const char *parse_number(int *dest, const char *s)
{
    if (s[1] == '.')
    {
        *dest = s[0] * 10 + s[2] - ('0' * 11);
        return s + 4;
    }
    if (s[2] == '.')
    {
        *dest = s[0] * 100 + s[1] * 10 + s[3] - ('0' * 111);
        return s + 5;
    }
    if (s[3] == '.')
    {
        *dest = s[0] * 1000 + s[1] * 100 + s[2] * 10 + s[4] - ('0' * 1111);
        return s + 6;
    }
}

int processPartData(unsigned char *leftBound)
{
    rax *rt = raxNew();

    char *file = "measurements.txt";

    int fd = open(file, O_RDONLY);
    if (!fd)
    {
        perror("error opening file");
        exit(EXIT_FAILURE);
    }

    struct stat sb;
    if (fstat(fd, &sb) == -1)
    {
        perror("error getting file size");
        exit(EXIT_FAILURE);
    }

    // mmap entire file into memory
    size_t sz = (size_t)sb.st_size;
    const char *data = mmap(NULL, sz, PROT_READ, MAP_SHARED, fd, 0);
    if (data == MAP_FAILED)
    {
        perror("error mmapping file");
        exit(EXIT_FAILURE);
    }

    raxIterator iter;
    raxStart(&iter, rt);
    raxSeek(&iter, "^", (unsigned char *)NULL, 1);

    int numele = rt->numele;
    raxFree(rt);
    return numele;
}

int main(int argc, char const *argv[])
{
    unsigned char *s[128];
    for (int i = 0; i < 128; i++)
    {
        s[i] = '0';
    }
    while (!processPartData(s) < MAX_KEY_CAPABILITY)
    {
    }

    return 0;
}
