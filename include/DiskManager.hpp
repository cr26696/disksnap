#ifndef DISKMANAGER_HPP
#define DISKMANAGER_HPP

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
#include "Object.hpp"
#include "Replica.hpp"
#include "Disk.hpp"
#include "PersuadeThread.hpp"
class DiskManager
{
private:
    static DiskManager *instance;
    int DiskNum, DiskVolume, HeadToken;
    std::vector<Disk> disks;
    std::vector<PersuadeThread> job_threads;
public:
    Object objects[MAX_OBJECT_NUM];//结构体数组，存放全部占内存大小 6*4*100k = 2.4M
    // std::unordered_set<int> canceled_reqs;
    // std::unordered_set<int> completed_reqs;
    // std::unordered_map<int, std::vector<int>> map_obj_diskid;
private:
    DiskManager(int DiskNum, int DiskVolume, int HeadToken);

public:
    static DiskManager &getInstance(int DiskNum, int DiskVolume, int HeadToken); // 传入磁盘数 磁盘存储空间大小 磁头移动速度
    static DiskManager &getInstance();

    std::vector<Disk*> get_disks(std::vector<int> disk_ids);
    Disk& get_disk(int disk_id);
    void store_obj(int id, int size, int tag); // 修改存储函数接口
    void remove_obj(int obj_id);
    void request_obj(int request_id,int object_id);
    vector<int> get_canceled_reqs_id();
    vector<int> get_complete_reqs_id();
    void clean();
    // static DiskManager* getInstance(int T, int M, int N, int V, int G);
    void end();
};

#endif // DISKMANAGER_HPP