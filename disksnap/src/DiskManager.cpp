// DiskManager.cpp
#include "DiskManager.hpp"
#include <map>
using namespace std;

DiskManager *DiskManager::instance = nullptr;

DiskManager::DiskManager(int DiskNum, int DiskVolume, int HeadToken, vector<int>& label_index)
    : DiskNum(DiskNum),
      DiskVolume(DiskVolume),
      HeadToken(HeadToken)
{   
    disks.resize(DiskNum, Disk(DiskVolume, HeadToken, 0 ,label_index));
    for (int i = 0; i < DiskNum; i++)
    {
        disks[i].id = i;
    }
};

DiskManager &DiskManager::getInstance(int DiskNum, int DiskVolume, int HeadToken, vector<int>& label_index)
{
    if (instance == nullptr)
    {
        instance = new DiskManager(DiskNum, DiskVolume, HeadToken, label_index); // 懒加载创建实例
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
void DiskManager::store_obj(int id, int size, int tag, const std::vector<int>& label_index)
{
    // vector<int> Doptions;//disk options
    // OPT 排除掉不可能存放的盘
    vector<int> tag_spaces;
    vector<int> all_spaces;
    struct ComparePair {
        bool operator()(const std::pair<int, int>& p1, const std::pair<int, int>& p2) {
            // 比较 pair.second，值大的优先级更高
            return p1.second < p2.second;
        }
    };
    priority_queue<pair<int, int>, vector<pair<int, int>>, ComparePair> spaces;
    for(int i = 0; i < DiskNum; i++){
        tag_spaces.push_back(disks[i].numberOfFreeSubBlocks(tag)); //要加入标签下的空闲值
        if(tag_spaces[i]< size){
            continue;
        }
        spaces.push(make_pair(i, tag_spaces[i]));
    }

    // 选3个子磁盘最大的存，不够就找磁盘容量大的存
    // TODO:通过读的频率来选择盘->负载平衡
    int seleted[3];
    int temp;
    if(spaces.size() >= 3) 
        temp = 3;
    else
        temp = spaces.size();
    for(int i = 0; i < temp; i++){
        seleted[i] = spaces.top().first;
        spaces.pop();
    }
    if(temp < 3){
        int count = 0;
        for(int i = 0; i < DiskNum; i++){
            //检查磁盘是否已经在 seleted 中
            bool is_selected = false;
            for(int j = 0; j < temp; j++){
                if(seleted[j] == i){
                    is_selected = true;
                    count++;
                    break;
                }
            }
            if(is_selected){
                continue; // 如果磁盘已经在 seleted 中，跳过
            }
            all_spaces.push_back(disks[i].numberOfFreeAllBlocks()); //要加入标签下的空闲值
            if(all_spaces[i - count]< size){
                continue;
            }
            spaces.push(make_pair(i, all_spaces[i - count]));
        }
        for(int i = temp; i < 3; i++){
            seleted[i] = spaces.top().first;
            spaces.pop();
        }
    }
    
    // 假设这里只有三个盘
    for (int i : seleted)
    {
        Replica *rep = new Replica(id, size, tag);
        disks[i].write_obj(rep, label_index);
        map_obj_diskid[id].push_back(i);
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
void DiskManager::find()
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
    for(int i=0;i<DiskNum;i++){
        disks[i].end();
    }
    completed_reqs.clear();
    canceled_reqs.clear();
}
