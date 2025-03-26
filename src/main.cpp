// main.cpp
#include <cstdio>
#include <cassert>
#include <cstdlib>

#include "MetaDefine.hpp"
#include "System.hpp"
using namespace std;

int main()
{
    int T, M, N, V, G;
    scanf("%d%d%d%d%d", &T, &M, &N, &V, &G);
    System &system = System::getInstance(T, M, N, V, G);
    system.run();

    return 0;
}