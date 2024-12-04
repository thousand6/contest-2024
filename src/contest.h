#include<stdio.h>

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


int max(int a, int b);
int min(int a, int b);
