#include <stdio.h>
#include <vector>

void work()
{
    std::vector<int> data(10);
    for (int i = 0; i < 11; ++i) {
        data[i] = i*2;
        printf("data[%d] = %d\n", i, data[i]);
    }
}
