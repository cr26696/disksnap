#include <cstdio>
#include <cassert>
#include <cstdlib>
#include <unistd.h>
#include <fstream>

#include "MetaDefine.hpp"
#include "System.hpp"
using namespace std;

int main()
{
    truncate("sorce_info.txt", 0);
    int T, M, N, V, G;
    scanf("%d%d%d%d%d", &T, &M, &N, &V, &G);
    System &system = System::getInstance(T, M, N, V, G);
    system.run();

    return 0;
}