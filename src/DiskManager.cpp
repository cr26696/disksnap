// DiskManager.cpp
#include <map>
#include "DiskManager.hpp"
#include "Scheduler.hpp"
using namespace std;

DiskManager *DiskManager::instance = nullptr;

DiskManager::DiskManager(int DiskNum, int DiskVolume, int HeadToken)
    : DiskNum(DiskNum),
      DiskVolume(DiskVolume),
      HeadToken(HeadToken)
{
    disks.reserve(DiskNum);

    for (int i = 0; i < DiskNum; i++)
    {
        disks.emplace_back(DiskVolume, HeadToken, i); // emplace_back调用构造函数直接存入，不触发拷贝
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
// 返回存储对象的信息
Object &DiskManager::store_obj(int id, int size, int tag)
{
    objects[id] = Object(id, size, tag);
    Object &object = objects[id];
    vector<int> Doptions; // disk options
    // OPT numberOfFreeBlocks_计算结果存储
    vector<int> spaces;
    // 过滤空间不足存储的盘
    for (int i = 0; i < DiskNum; i++)
    {
        spaces.push_back(disks[i].numberOfFreeBlocks_());
        if (spaces[i] < size)
        {
            continue;
        }
        Doptions.push_back(i);
    }
    // 排序
    sort(Doptions.begin(), Doptions.end(), [&](int a, int b)
         { return spaces[a] > spaces[b]; });
    // 假设排序后前三个盘空间最大
    for (int i = 0; i < REP_NUM; i++)
    {
        int disk_id = Doptions[i];
        object.diskid_replica[i] = disk_id;
        // Replica *rep = new Replica{id, size, tag};
        disks[disk_id].wrt_obj(object);
    }
    bool mute = false;
    // 上报存储结果
    if (!mute)
    {
        printf("%d\n", id);
        for (int i = 0; i < REP_NUM; i++)
        {
            int disk_id = Doptions[i];
            Disk &d = disks[disk_id];
            printf("%d", disk_id + 1);
            // 每个块的存储地址
            vector<int> addrs = d.get_store_pos(id);
            string s;
            for (int k = 0; k < size; k++)
            {
                // s += " " + addrs[k];
                printf(" %d", addrs[k] + 1);
            }
            // printf("%s\n", s.c_str());
            printf("\n");
        }
    }
    return object;
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
        disks[disk_id].del_obj(info);
    }
    // objects[obj_id]= //这里直接没清理的必要了
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
// vector<Disk *> DiskManager::get_disks(vector<int> disk_ids)
// {
//     vector<Disk *> ret_disks;
//     for (int disk_id : disk_ids)
//     {
//         ret_disks.push_back(&disks[disk_id]);
//     }
//     return ret_disks;
// }

// 清除帧结束，调用清理
void DiskManager::end()
{
    for (int i = 0; i < DiskNum; i++)
    {
        disks[i].end();
        // job_threads[i].end();
    }
    // completed_reqs.clear();
    // canceled_reqs.clear();
}
