// System.hpp
#ifndef SYSTEM_HPP
#define SYSTEM_HPP

#include "DiskManager.hpp"
#include "Replica.hpp" // 新增
#include <vector>

class System
{
private:
    int TimeStampNum;
    int Tags;
    int DiskNum;
    int DiskVolume;
    int Token;
    DiskManager diskManager;

    // 私有构造函数
    System(int T, int M, int N, int V, int G);

    // 删除复制构造函数和赋值操作符
    System(const System&) = delete;
    System& operator=(const System&) = delete;

    // 静态实例指针
    static System* instance;

public:
    // 获取单例实例的静态方法
    static System* getInstance(int T, int M, int N, int V, int G);

    void run();
    void timestamp_action();
    void delete_action();
    void read_action();
    void write_action();
    std::vector<int> select_disks_for_replica(int id);
};

#endif // SYSTEM_HPP