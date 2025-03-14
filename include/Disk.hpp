// Disk.hpp
#ifndef DISK_HPP
#define DISK_HPP

#include <vector>
#include <list>
#include <string>
#include <iostream>
#include <cassert>
#include <algorithm>
#include <cmath>
#include "MetaDefine.hpp"
#include <unordered_map>
#include "RequestManager.hpp"
#include "ObjectManager.hpp"

enum DiskOp
{
    PASS,
    JUMP,
    READ,
};

class Disk
{
private:
    int head;             // 初始为0 如size == 8000 可能的值为0~7999
    int head_s;           // 磁头上个操作 -1初始化 0刚jump过 >0
    int sizeV;            // 磁盘容量
    std::vector<int> blocks; // 考虑这里就存 obj_id吗，需不需要其他信息？
    int tokenG;            // 当前时间片总可用token数
    int elapsed;          // 当前时间片已用token数
    int phase_end;        // 是否结束当前阶段

    std::list<pair<int, int>> free_blocks;// 存储空闲块区间
    // 会移动head JUMP需传入跳转地址 PASS传入距离 READ无视参数可填0
    // 返回操作是否成功 pass 0也算成功
    bool operate(DiskOp op, int param);

    void op_end();

public:
    Disk(int V, int G); // 构造函数

    // void init(int G, int V);
    void delete_obj(int *units, int object_size);
    void write_obj(int object_id, int *obj_units, int object_size);
    void task(std::vector<int> input_target, int disk_id);
    int numberOfFreeBlocks_();

    // void store(int id, int tag, int size);
};

#endif // DISK_HPP