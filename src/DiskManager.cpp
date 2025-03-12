// DiskManager.cpp
#include "DiskManager.hpp"
#include <cstdio>
#include <cassert>
#include <cstdlib>
#include <vector>
#include <string>
#include <algorithm>
#include <unordered_set>
#include <iostream>
#include <math.h>
#include "MetaDefine.hpp"

using namespace std;

DiskManager::DiskManager(int T, int M, int N, int V, int G)
    : T(T), M(M), N(N), V(V), G(G)
{
    for (auto &D : disks)
    {
        D.init();
    }
}

void DiskManager::clean()
{
    for (auto &obj : object)
    {
        for (int i = 1; i <= REP_NUM; i++)
        {
            if (obj.unit[i] == nullptr)
                continue;
            free(obj.unit[i]);
            obj.unit[i] = nullptr;
        }
    }
}

// 修改存储函数实现
void DiskManager::store(int id, int tag, int size) {
    // TODO: 分析所有磁盘的存储情况，选择最适合的磁盘
    vector<Disk*> selected_disks;
    for (int i = 0; i < N && selected_disks.size() < REP_NUM; i++) {
        if (disks[i].hasSpace(size)) {
            selected_disks.push_back(&disks[i]);
        }
    }

    if (selected_disks.size() < REP_NUM) {
        printf("Failed to find enough disks to store replica %d\n", id);
        return;
    }

    for (int i = 0; i < REP_NUM; i++) {
        selected_disks[i]->store(id, tag, size); // 调用最适合的磁盘的store方法
    }
}