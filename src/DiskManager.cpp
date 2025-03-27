// DiskManager.cpp
#include <map>
#include "DiskManager.hpp"
#include "Scheduler.hpp"
using namespace std;

DiskManager *DiskManager::instance = nullptr;

DiskManager::DiskManager(int DiskNum, int DiskVolume, int HeadToken, vector<double> &tag_ratio)
    : DiskNum(DiskNum),
      DiskVolume(DiskVolume),
      HeadToken(HeadToken)
{
    disks.reserve(DiskNum);
    for (int i = 0; i < DiskNum; i++)
    {
        disks.emplace_back(DiskVolume, HeadToken, i, tag_ratio); // emplace_back调用构造函数直接存入，不触发拷贝
    }
};

DiskManager &DiskManager::getInstance(int DiskNum, int DiskVolume, int HeadToken, vector<double> &tag_ratio)
{
    if (instance == nullptr)
    {
        instance = new DiskManager(DiskNum, DiskVolume, HeadToken, tag_ratio);
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
// 存放时要输出存储信息： 对象id 各副本存放位置
// 返回存储对象的信息
string DiskManager::store_obj(int id, int size, int tag)
{
    objects[id] = Object(id, size, tag);
    Object &object = objects[id];
    string s;
    for (int i = 0; i < REP_NUM; i++)
    {
        vector<Disk *> DiskOptions;
        DiskOptions.reserve(DiskNum);
        // 过滤
        for (int j = 0; j < DiskNum; j++)
        {
            // 过滤掉空间不足、已使用的盘
            if (disks[j].getAllSpace() < size || disks[j].get_replica(id) != nullptr)
                continue;
            DiskOptions.push_back(&disks[j]);
        }
        // 接下来选出最合适存放的盘
        // 先找对应tag空间最大的盘
        int idx_tag_space = 0; // tag区域空间最大的盘的下标
        int idx_all_space = 0; // 全部空闲空间最大的盘的下标
        for (int j = 1; j < DiskOptions.size(); j++)
        {
            if (DiskOptions[j]->getRegionSpace(tag) > DiskOptions[idx_tag_space]->getRegionSpace(tag))
                idx_tag_space = j;
            if (DiskOptions[j]->getAllSpace() > DiskOptions[idx_all_space]->getAllSpace())
                idx_all_space = j;
        }

        Disk *disk;
        if (DiskOptions[idx_tag_space]->getRegionSpace(tag) < size)
            disk = DiskOptions[idx_all_space];
        else
            disk = DiskOptions[idx_tag_space]; // 对应tag区域足够存放 直接存入
        s += disk->wrt_replica(object) + "\n";//盘号（注意从1开始) + 对象各块存储位置 + 换行
        object.diskid_replica[i] = disk->id;
    }
    return s;
}
// 移除3个obj_id的副本 返回关联的请求取消数量
void DiskManager::remove_obj(int obj_id)
{
    Scheduler &SD = Scheduler::getInstance();
    Object &info = objects[obj_id];
    for (int i = 0; i < REP_NUM; i++)
    {
        int disk_id = info.diskid_replica[i];
        PersuadeThread &t = SD.get_disk_thread(disk_id);
        t.rmv_req(info);
        disks[disk_id].del_replica(info);
    }
}

Disk &DiskManager::get_disk(int disk_id)
{
    return disks[disk_id];
}

// 清除帧结束，调用清理
void DiskManager::end()
{
    for (int i = 0; i < DiskNum; i++)
    {
        disks[i].end();
    }
}
