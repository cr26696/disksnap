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
#include "Object.hpp"
#include "DiskRegion.hpp"
enum DiskOp
{
    PASS,
    JUMP,
    READ,
};

class Disk
{
    friend class PersuadeThread;
private:
    int head_s;                                               // 磁头上个操作 -1初始化 0刚jump过 >0
    int head;       // 初始为0 如size == 8000 可能的值为0~7999
    bool phase_end; // 是否结束当前阶段
    int elapsed;    // 当前时间片已用token数
    Replica *replicas[MAX_OBJECT_NUM]={nullptr};//已存储的所有副本
    std::optional<std::pair<int, int>> blocks[MAX_DISK_SIZE];//磁盘各存储单元
    std::vector<DiskRegion> regions;//磁盘各区域
    int freeBlocks;
public:
    const int id;
    const int volume;     // 磁盘容量
    const int tokenG;     // 当前时间片总可用token数
public:
    Disk(int V, int G, int id,std::vector<double>& tag_ratio); // 构造函数
    void end();
    void op_end();
    bool operate(DiskOp op, int param);

    Replica *get_replica(int obj_id);
    void del_replica(Object &info);
    std::string wrt_replica(Object &info);
    int getRegionSpace(int tag);
    int getAllSpace();
};

#endif // DISK_HPP