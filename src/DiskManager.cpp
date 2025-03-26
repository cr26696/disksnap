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
Object &DiskManager::store_obj(int id, int size, int tag)
{
    objects[id] = Object(id, size, tag);
    Object &object = objects[id];
    for (int i = 0; i < REP_NUM; i++)
    {
        vector<Disk *> DiskOptions;
        DiskOptions.reserve(DiskNum);
        for (int j = 0; j < DiskNum; j++)
        {
            // 过滤掉空间不足、已使用的盘
            if (disks[j].getFreeBlocks() < size || disks[j].get_replica(id) != nullptr)
                continue;
            DiskOptions.push_back(&disks[j]);
        }
        // 接下来选出最合适存放的盘
        // 先找空间最大的盘
        int ideal_id = 0;
        for (int j = 1;j< DiskOptions.size();j++){
            if (DiskOptions[j]->getFreeBlocks() > DiskOptions[ideal_id]->getFreeBlocks())
                ideal_id = j;
        }
        Disk *ideal_disk = DiskOptions[ideal_id];
        ideal_disk->wrt_rep(object);
        //TODO 返回写入信息
    }
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
        disks[disk_id].del_rep(info);
    }
}
void DiskManager::request_obj(int request_id, int object_id)
{
    // // 选择最适合的disk放入任务
    // // 找到能响应请求的disk
    // //  DiskManager DM = DiskManager::getInstance();
    // Object &obj = objects[object_id];
    // int ideal_id = obj.diskid_replica[0];
    // // 从第一个副本所在的磁盘线程开始比较
    // for (int i = 1; i < REP_NUM; i++)
    // {
    //     int compare_id = obj.diskid_replica[i];
    //     if (job_threads[compare_id].task_requests.size() < job_threads[ideal_id].task_requests.size())
    //     {
    //         ideal_id = i;
    //     }
    // }
    // // 选好线程，调用添加任务函数
    // job_threads[ideal_id].add_req(request_id, objects[object_id]);
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
