// Scheduler.cpp
#include "Scheduler.hpp"
#include "DiskManager.hpp"
using namespace std;

Scheduler* Scheduler::instance = nullptr;

Scheduler& Scheduler::getInstance()
{
    if (instance == nullptr)
    {
        instance = new Scheduler();
    }
    return *instance;
}

bool Scheduler::add_request(int req_id,int obj_id)
{   
    lock_guard<std::mutex> lock(mutex_);
    //TODO 选择最适合的disk放入任务
    //找到能响应请求的disk
    DiskManager DM = DiskManager::getInstance();
    vector<Disk*> disks = DM.get_disks(DM.map_obj_diskid[req_id]);
    //选出任务最少的disk
    int ideal_disk_id = 0;
    int min_job_count = disks[0]->job_count;
    for(int i=1;i<3;i++){
        if(disks[i]->job_count < min_job_count){
            ideal_disk_id = i;
            min_job_count = disks[i]->job_count;
        }
    }
    Request* req = new Request(req_id,obj_id);
    disks[ideal_disk_id]->add_req(req);
    // vector<Request>& requests = RequestManager::getInstance()->getRequests();
    // int object_id = requests[req_id].object_id;
    // active_requests[object_id].push_back(req_id);
    return true; // 假设添加总是成功
}

bool Scheduler::del_request(int req_id)
{
    lock_guard<std::mutex> lock(mutex_);
    vector<Request>& requests = RequestManager::getInstance()->getRequests();
    int object_id = requests[req_id].object_id;
    auto& req_list = active_requests[object_id];
    active_requests.find(req_id);
    auto it = std::find(req_list.begin(), req_list.end(), req_id);
    if (it != req_list.end())
    {
        req_list.erase(it);
        if (req_list.empty())
        {
            active_requests.erase(object_id);
        }
        return true;
    }
    return false;
}

vector<int> Scheduler::get_task_for_disk(int disk_id)
{
    lock_guard<std::mutex> lock(mutex_);
    vector<int> target;
    vector<Request>& requests = RequestManager::getInstance()->getRequests();
    vector<Object>& objects = ObjectManager::getInstance()->getObjects();
    for (const auto& [object_id, req_ids] : active_requests)
    {
        Request req = requests[req_ids[0]]; // 假设第一个请求代表整个对象
        Object obj = objects[req.object_id];

        for (int i = 1; i <= 3; i++)
        {
            if (obj.replica[i] == disk_id)
            {
                int *units = obj.unit[i];
                for (int j = 1; j <= obj.size; j++) // 修改变量名避免冲突
                {
                    target.push_back(units[j]);
                }
                break;
            }
        }
    }
    if (!target.empty()) // 仅在非空时排序
    {
        sort(target.begin(), target.end());
    }
    DEBUG_PRINT(target.size());
    return target;
}

void Scheduler::req_upload()
{
    DiskManager& DM = DiskManager::getInstance();
    lock_guard<std::mutex> lock(mutex_);
    DM.read();
    int complete_num = 0;
    string info = "";
    // bool complete_flag;
    // vector<Request>& requests = RequestManager::getInstance()->getRequests();
    // vector<Object>& objects = ObjectManager::getInstance()->getObjects();
    // for (const auto& [object_id, req_ids] : active_requests)
    // {
    //     for (int actreq_id : req_ids)
    //     {
    //         complete_flag = true; // 初始化为true
    //         for (int i = 1; i <= objects[requests[actreq_id].object_id].size; i++)
    //         {
    //             if (requests[actreq_id].complete[i] != true)
    //                 complete_flag = false; // 如果有不完整的，设置为false
    //         }
    //         if (complete_flag)
    //         {
    //             complete_num++;
    //             del_request(actreq_id);
    //             requests[actreq_id].is_done = true;
    //             info += to_string(actreq_id) + "\n"; // 转换为字符串
    //         }
    //     }
    // }
    printf("%d\n",DM.completed_reqs.size());
    for(int req_id:DM.completed_reqs){
        info+= to_string(req_id)+"\n";
    }
    printf("%s",info.c_str());
}

unordered_map<int, std::vector<int>>& Scheduler :: get_active_requests(){
    return active_requests;
}