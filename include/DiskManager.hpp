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
    std::vector<Disk> disks;

public:
    int DiskNum, DiskVolume, HeadToken;
    Object objects[MAX_OBJECT_NUM]; // 结构体数组，存放全部占内存大小 6*4*100k = 2.4M
private:
    DiskManager(int DiskNum, int DiskVolume, int HeadToken, vector<double> &tag_ratio);

public:
    static DiskManager &getInstance(int DiskNum, int DiskVolume, int HeadToken, vector<double> &tag_ratio); // 传入磁盘数 磁盘存储空间大小 磁头移动速度
    static DiskManager &getInstance();

    Disk &get_disk(int disk_id);
    string store_obj(int id, int size, int tag);
    void remove_obj(int obj_id);
    void request_obj(int request_id, int object_id);

    void end();
};

#endif // DISKMANAGER_HPP