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
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <optional>
#include "MetaDefine.hpp"
#include "Request.hpp"
#include "Replica.hpp"
#include "Unit.hpp"
#include "Object.hpp"

enum DiskOp
{
    PASS,
    JUMP,
    READ,
};

class Disk
{
private:
    int head_s;                                               // 磁头上个操作 -1初始化 0刚jump过 >0
    std::list<std::pair<int, int>> free_blocks; // 空闲块的起始 结束地址
    Replica *replicas[MAX_DISK_SIZE]={nullptr};

public:
    int id;
    int head;       // 初始为0 如size == 8000 可能的值为0~7999
    int volume;     // 磁盘容量
    int elapsed;    // 当前时间片已用token数
    int tokenG;     // 当前时间片总可用token数
    bool phase_end; // 是否结束当前阶段
    std::optional<std::pair<int, int>> blocks[MAX_DISK_SIZE];
public:
    Disk(int V, int G, int id,vector<double>& tag_ratio); // 构造函数
    void op_end();
    bool operate(DiskOp op, int param);

    void del_obj(Object &info);
    void wrt_obj(Object &info);
    std::pair<int, int> get_block(int addr);
    Replica *get_replica(int obj_id);
    std::vector<int> get_store_pos(int obj_id);
    void end();
    // void init(int G, int V);
    // void task(std::vector<int> input_target, int disk_id);
    int numberOfFreeBlocks_();

    // void store(int id, int tag, int size);
};

#endif // DISK_HPP