// Disk.hpp
#ifndef DISK_HPP
#define DISK_HPP

#define COMPLET_MODE 1
#define MAXFREE_MODE 2
#define DISCRET_MODE 3

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
    friend class DiskManager;
    friend class PersuadeThread;
    friend class Scheduler;
private:
    int head_s;                                               // 磁头上个操作 -1初始化 0刚jump过 >0
    // std::optional<std::pair<int, int>> blocks[MAX_DISK_SIZE]; // 对象id 对象的块序号
    // std::unordered_map<int, std::unordered_set<Request *>> map_obj_request; // obj_id <-> 请求指针set
    std::list<std::pair<int, int>> free_blocks; // 空闲块的起始 结束地址
    Replica *replicas[MAX_OBJECT_NUM]={nullptr};

public:
    int id;
    int head;       // 初始为0 如size == 8000 可能的值为0~7999
    int volume;     // 磁盘容量
    int elapsed;    // 当前时间片已用token数
    int tokenG;     // 当前时间片总可用token数
    bool phase_end; // 是否结束当前阶段
    // int job_count;
    // std::unordered_map<int, Replica *> map_obj_replica;     // obj_id <-> 副本指针
    std::unordered_map<int, std::vector<int>> map_obj_part_addr; // obj_id <-> 各块存储地址
    // std::vector<int> completed_reqs; // 帧结束清空 记录完成请求的id
    std::optional<std::pair<int, int>> blocks[MAX_DISK_SIZE];
public:
    Disk(int V, int G, int id); // 构造函数
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