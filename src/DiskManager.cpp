// DiskManager.cpp
#include "DiskManager.hpp"
#include <map>
using namespace std;

DiskManager *DiskManager::instance = nullptr;

DiskManager::DiskManager(int DiskNum, int DiskVolume, int HeadToken)
    : DiskNum(DiskNum),
      DiskVolume(DiskVolume),
      HeadToken(HeadToken)
{
    disks.resize(DiskNum, Disk(DiskVolume, HeadToken, 0));
    for (int i = 0; i < DiskNum; i++)
    {
        disks[i].id = i;
    }
};

DiskManager &DiskManager::getInstance(int DiskNum, int DiskVolume, int HeadToken)
{
    if (instance == nullptr)
    {
        instance = new DiskManager(DiskNum, DiskVolume, HeadToken); // 懒加载创建实例
    }
    return *instance;
}
DiskManager &DiskManager::getInstance()
{
    if (instance == nullptr)
    {
        throw std::runtime_error("DiskManager has not been initialized with parameters.");
    }
    return *instance;
}

// std::vector<Disk> &DiskManager::getDisks()
// {
//     return DiskManager::disks;
// }

void DiskManager::clean()
{
    // vector<Object> &objects = ObjectManager::getInstance()->getObjects();
    // for (auto &obj : objects)
    // {
    //     for (int i = 1; i <= REP_NUM; i++)
    //     {
    //         if (obj.unit[i] == nullptr)
    //             continue;
    //         free(obj.unit[i]);
    //         obj.unit[i] = nullptr;
    //     }
    // }
}
// 存放时要输出存储信息： 对象id 各副本存放位置
void DiskManager::store_obj(int id, int size, int tag)
{
    vector<int> store_destination;
    for (int i = 0; i < DiskNum; i++)
    {
        store_destination.push_back(i);
    }
    // TODO 两步
    // 排除掉不可能存放的盘

    // 选出3个最适合存放的盘
    // 存放
    // 假设这里只有三个盘
    for (int i : store_destination)
    {
        Replica *rep = new Replica(id, size, tag);
        disks[i].write_obj(rep);
    }

    // 上报存储结果
    printf("%d\n", id);
    for (int i = 0; i < store_destination.size(); i++)
    {
        int disk_id = store_destination[i];
        map_obj_diskid[id][i] = disk_id;
        printf("%d", disk_id);
        Disk &d = disks[disk_id];
        // 每个块的存储地址
        vector<int> &addrs = d.map_obj_part_addr[id];
        for (int k = 0; k < addrs.size(); k++)
        {
            printf(" %d", addrs[k]);
        }
        printf("\n");
    }
}
// 移除3个obj_id的副本 返回联系的请求取消数量
void DiskManager::remove_obj(int obj_id)
{
    for (int i = 0; i < 3; i++)
    {
        Disk &d = disks[map_obj_diskid[obj_id][i]];
        // 磁盘中未存放obj_id的副本
        if (d.map_obj_replica.find(obj_id) == d.map_obj_replica.end())
        {
            continue; // 去下个磁盘
        }
        d.delete_obj(obj_id);
    }
    map_obj_diskid.erase(obj_id);
}
void DiskManager::read()
{
    for (int i = 0; i < DiskNum; i++)
    {
        disks[i].find();
    }
}
Disk &DiskManager::get_disk(int disk_id)
{
    return disks[disk_id];
}
vector<Disk *> DiskManager::get_disks(vector<int> disk_ids)
{
    vector<Disk *> ret_disks;
    for (int disk_id : disk_ids)
    {
        ret_disks.push_back(&disks[disk_id]);
    }
    return ret_disks;
}
// 清除一些单帧内的变量
void DiskManager::end()
{
    completed_reqs.clear();
    canceled_reqs.clear();
}
