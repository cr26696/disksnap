#ifndef REPLICA_HPP
#define REPLICA_HPP

#include <vector>
#include "Unit.hpp"
class Replica
{
public:
    int id, size, tag;
    std::vector<Unit> Units;
    // std::vector<int> parts; // 各部分存在当前盘的地址

public:
    Replica(int id, int size, int tag);
    ~Replica();
    // std::vector<Unit *> &getUnits();
    // 返回副本各块存放磁盘的地址（序号）
    // std::vector<int> &getPart();
    // const std::vector<int> &getData() const;
};
#endif // REPLICA_HPP