#ifndef DISKMANAGER_HPP
#define DISKMANAGER_HPP

#include <vector>
#include "Replica.hpp" // 新增

class Disk {
public:
    void init() {
        // 初始化磁盘
    }

    bool hasSpace(int size) {
        // 检查磁盘是否有足够的空间
        return true; // 伪代码实现
    }

    void store(int id, int tag, int size) {
        // 存储对象的逻辑
        // 伪代码实现
    }
};

class DiskManager {
private:
    int T, M, N, V, G;
    std::vector<Disk> disks;
    std::vector<Object> object;

public:
    DiskManager(int T, int M, int N, int V, int G);
    void clean();
    void store(int id, int tag, int size); // 修改存储函数接口
};

#endif // DISKMANAGER_HPP