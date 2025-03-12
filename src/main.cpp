// main.cpp
#include <cstdio>
#include <cassert>
#include <cstdlib>
#include <vector>
#include <string>
#include <algorithm>
#include <unordered_set>
#include <iostream>
#include <math.h>
#include "System.hpp"
#include "MetaDefine.hpp"

using namespace std;

int main()
{
    int T, M, N, V, G;
    scanf("%d%d%d%d%d", &T, &M, &N, &V, &G);

    System* system = System::getInstance(T, M, N, V, G);
    system->run();

    return 0;
}