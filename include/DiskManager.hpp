#ifndef DISKMANAGER_HPP
#define DISKMANAGER_HPP

#include <vector>
#include "Replica.hpp" // 新增
#include "Object.hpp"
#include "Disk.hpp"
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

// 为什么重复定义了一个Disk类？
// class Disk {
// public:
//     void init() {
//         // 初始化磁盘
//     }

//     bool hasSpace(int size) {
//         // 检查磁盘是否有足够的空间
//         return true; // 伪代码实现
//     }

//     void store(int id, int tag, int size) {
//         // 存储对象的逻辑
//         // 伪代码实现
//     }
// };

class DiskManager
{
private:
    static DiskManager *instance;
    std::vector<Disk> disks;

public:
    int DiskNum, DiskVolume, HeadToken;
    std::unordered_set<int> canceled_reqs;
    std::unordered_set<int> completed_reqs;
    std::unordered_map<int, std::vector<int>> map_obj_diskid;
private:
    DiskManager(int DiskNum, int DiskVolume, int HeadToken);

public:
    static DiskManager &getInstance(int DiskNum, int DiskVolume, int HeadToken); // 传入磁盘数 磁盘存储空间大小 磁头移动速度
    static DiskManager &getInstance();

    std::vector<Disk*> get_disks(std::vector<int> disk_ids);
    Disk& get_disk(int disk_id);
    void store_obj(int id, int size, int tag); // 修改存储函数接口
    void remove_obj(int obj_id);
    void read();
    void clean();
    void store(int id, int tag, int size); // 修改存储函数接口
    // static DiskManager* getInstance(int T, int M, int N, int V, int G);
    void end();
};

#endif // DISKMANAGER_HPP