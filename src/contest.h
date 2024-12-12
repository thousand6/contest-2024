#include<stdio.h>
#include <fcntl.h>

typedef struct Group
{
    unsigned long sum;
    int max;
    int min;
    int count;
} Group;

typedef struct FileEntry
{
    FILE *file;
    char *name;
} FileEntry;

typedef struct FileMapContainer
{
    int fd;
    size_t fileSize;
    char array[150];
    char *data;
    off_t offset;
    size_t mapSize;
    char *origin;
} FileMapContainer;




int max(int a, int b);
int min(int a, int b);
