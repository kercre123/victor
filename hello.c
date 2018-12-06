#include <stdio.h>
#include <string.h>

int main(int argc, char* argv[])
{
    char* s = strdup("hello, world");
    printf("%s\n", s);

    int data[10];
    for (int i = 0; i < 11; ++i) {
        data[i] = i*2;
        printf("data[%d] = %d\n", i, data[i]);
    }

    return 0;
}
