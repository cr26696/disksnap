#ifndef REPLICA_HPP
#define REPLICA_HPP

#include <vector>
#include "Unit.hpp"
#include <memory>
struct Replica
{
    // std::weak_ptr<Object> info;
    int addr_part[5];//对象各部分存储的磁盘地址 初始化值-1
    Object& info;
    // Unit Units[5];
    // std::vector<Request*> related_requests;
    // std::vector<int> parts; // 各部分存在当前盘的地址
    // public:
    // Replica(int id, int size, int tag);
    // ~Replica();
    // std::vector<Unit *> &getUnits();
    // 返回副本各块存放磁盘的地址（序号）
    // std::vector<int> &getPart();
    // const std::vector<int> &getData() const;
};
#endif // REPLICA_HPP