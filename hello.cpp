#include <stdio.h>
#include <string>
#include <vector>

extern void work();

int main(int argc, char* argv[])
{
    std::string* s = new std::string("hello, world");
    printf("%s\n", s->c_str());

    //std::vector<int> data(10);
    //for (int i = 0; i < 11; ++i) {
    //    data[i] = i*2;
    //    printf("data[%d] = %d\n", i, data[i]);
    //}

    work();

    return 0;
}

