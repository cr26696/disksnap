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
    int phase_end;                                                // 是否结束当前阶段
    std::vector<Unit *> blocks;                                   // 考虑这里就存 obj_id吗，需不需要其他信息？
    std::unordered_map<int, std::unordered_set<Request *>> map_obj_request; // obj_id <-> 请求指针set
    std::list<pair<int, int>> free_blocks;
public:
    int id;
    int job_count;
    std::unordered_map<int, Replica *> map_obj_replica;     // obj_id <-> 副本指针
    std::unordered_map<int, std::vector<int>> map_obj_part_addr; // obj_id <-> 各块存储地址
    std::vector<int> completed_reqs;                   // 帧结束清空 记录完成请求的id
private:
    bool operate(DiskOp op, int param);
    void op_end();

public:
    Disk(int V, int G,int id); // 构造函数

    void del_obj(int obj_id);
    void wrt_obj(Replica *replica);
    // void write_obj(int object_id, int *obj_units, int size);
    void add_req(Request *);
    void find();
    void end();

    // void init(int G, int V);
    void delete_obj(int *units, int object_size);
    void write_obj(int object_id, int *obj_units, int object_size);
    int numberOfFreeBlocks_();

    // void store(int id, int tag, int size);
};

#endif // DISK_HPP