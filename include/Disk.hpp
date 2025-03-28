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
#include "Object.hpp"
#include "DiskRegion.hpp"

struct DiskRegion;


enum DiskOp
{
    PASS,
    JUMP,
    READ,
};
struct Block
{
	int obj_id = -1; // 对象id
	int part = -1;	 // 对象副本的第几块
	bool start = false;
	bool end = false;
	bool used = false;
};
class Disk
{
    friend class DiskManager;
    friend class PersuadeThread;
    friend class Scheduler;
private:
    int head_s;                                               // 磁头上个操作 -1初始化 0刚jump过 >0
    int head;       // 初始为0 如size == 8000 可能的值为0~7999
    int elapsed;    // 当前时间片已用token数
    int freeBlocks;
    Replica *replicas[MAX_OBJECT_NUM]={nullptr};//已存储的所有副本
    Block blocks[MAX_DISK_SIZE];//磁盘各存储单元
    std::vector<DiskRegion> regions;//磁盘各区域
    std::string upload_info;
    bool phase_end; // 是否结束当前阶段
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
    int get_regionIndix(int addr);
};

#endif // DISK_HPP