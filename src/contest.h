#ifndef CONTEST_H
#define CONTEST_H

typedef struct Group
{
    unsigned long sum;
    int max;
    int min;
    int count;
} Group;


int max(int a, int b);
int min(int a, int b);

#endif