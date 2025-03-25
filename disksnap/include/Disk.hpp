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
#include <unordered_set>
#include <utility>
#include "Request.hpp"
#include "Replica.hpp"
#include "Unit.hpp"

enum DiskOp
{
    PASS,
    JUMP,
    READ,
};

class Disk
{
private:
    int head;                                                     // 初始为0 如size == 8000 可能的值为0~7999
    int head_s;                                                   // 磁头上个操作 -1初始化 0刚jump过 >0
    int volume;                                                   // 磁盘容量
    int tokenG;                                                   // 当前时间片总可用token数
    int elapsed;                                                  // 当前时间片已用token数
    bool phase_end;                                                // 是否结束当前阶段
    std::vector<Unit *> blocks;                                   // 考虑这里就存 obj_id吗，需不需要其他信息？
    // std::unordered_map<int, std::unordered_set<Request *>> map_obj_request; // obj_id <-> 请求指针set
    std::unordered_map<int, std::vector<Request*>> map_obj_request;
    std::vector<std::list<std::pair<int, int>>> free_blocks;
    // std::unordered_map<int, std::vector<pair<int, int>>> free_blocks;
    std::vector<int> subdisk_free_capacity;
public:
    int id;
    int job_count;
    int free_capacity;
    std::unordered_map<int, Replica *> map_obj_replica;     // obj_id <-> 副本指针
    std::unordered_map<int, std::vector<int>> map_obj_part_addr; // obj_id <-> 各块存储地址
    std::vector<int> completed_reqs;                   // 帧结束清空 记录完成请求的id
private:
    bool operate(DiskOp op, int param);
    void op_end();

public:
    Disk(int volume, int G, int id, const std::vector<int>& label_index); // 构造函数

    void del_obj(int obj_id);
    void wrt_obj(Replica *replica);
    void add_req(int req_id,int obj_id);
    void find();
    void end();

    // void init(int G, int V);
    void delete_obj(int object_id);
    void write_obj(Replica *replica, const std::vector<int>& label_index);
    void task(std::vector<int> input_target, int disk_id);
    // int numberOfFreeBlocks_(int tag, const std::vector<int>& label_index);
    int numberOfFreeAllBlocks();
    int numberOfFreeSubBlocks(int tag);
    // void store(int id, int tag, int size);
};

#endif // DISK_HPP