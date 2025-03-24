// Scheduler.cpp
#include "Scheduler.hpp"
#include "DiskManager.hpp"
using namespace std;

Scheduler::Scheduler()
{
    DiskManager &DM = DiskManager::getInstance();
    int diskNum = DM.DiskNum;
    for (int i = 0; i < diskNum; i++)
    {
        job_threads.push_back(&DM.get_disk(i));
    }
}
Scheduler *Scheduler::instance = nullptr;

Scheduler &Scheduler::getInstance()
{
    if (instance == nullptr)
    {
        instance = new Scheduler();
    }
    return *instance;
}

PersuadeThread &Scheduler::get_disk_thread(int thread_index)
{
    // TODO: insert return statement here
    return job_threads[thread_index];
}

bool Scheduler::add_request(int req_id, int obj_id)
{
    // //选择最适合的disk放入任务
    // //找到能响应请求的disk
    // DiskManager DM = DiskManager::getInstance();
    // Object &obj = DM.objects[obj_id];
    // vector<Disk*> disks;
    // for(int i=0;i<REP_NUM;i++){
    //     int disk_id = obj.diskid_replica[i];
    //     disks.push_back(DM.get_disk(disk_id));
    // }
    // = DM.get_disks(DM.map_obj_diskid[req_id]);
    // //选出任务最少的disk
    // int ideal_disk_id = 0;
    // int min_job_count = disks[0]->job_count;
    // for(int i=1;i<3;i++){
    //     if(disks[i]->job_count < min_job_count){
    //         ideal_disk_id = i;
    //         min_job_count = disks[i]->job_count;
    //     }
    // }
    // Request* req = new Request(req_id,obj_id);
    // disks[ideal_disk_id]->add_req(req);

    // 选择最适合的disk放入任务
    // 找到能响应请求的disk
    DiskManager &DM = DiskManager::getInstance();
    Object &obj = DM.objects[obj_id];
    int ideal_id = obj.diskid_replica[0];
    // 从第一个副本所在的磁盘线程开始比较
    for (int i = 1; i < REP_NUM; i++)
    {
        int compare_id = obj.diskid_replica[i];
        int current_size = job_threads[ideal_id].task_requests.size();
        int compare_size = job_threads[compare_id].task_requests.size();
        if (compare_size < current_size)
        {
            ideal_id = compare_id;
        }
    }
    // 选好线程，调用添加任务函数
    job_threads[ideal_id].add_req(req_id, obj);

    return true; // 假设添加总是成功
}

bool Scheduler::del_request(int req_id)
{
    lock_guard<std::mutex> lock(mutex_);
    vector<Request> &requests = RequestManager::getInstance()->getRequests();
    int object_id = requests[req_id].object_id;
    auto &req_list = active_requests[object_id];
    if (active_requests.find(req_id) == active_requests.end())
        return false;
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

// vector<int> Scheduler::get_task_for_disk(int disk_id)
// {
//     lock_guard<std::mutex> lock(mutex_);
//     vector<int> target;
//     vector<Request>& requests = RequestManager::getInstance()->getRequests();
//     vector<Object>& objects = ObjectManager::getInstance()->getObjects();
//     for (const auto& [object_id, req_ids] : active_requests)
//     {
//         Request req = requests[req_ids[0]]; // 假设第一个请求代表整个对象
//         Object obj = objects[req.object_id];

//         for (int i = 1; i <= 3; i++)
//         {
//             if (obj.replica[i] == disk_id)
//             {
//                 int *units = obj.unit[i];
//                 for (int j = 1; j <= obj.size; j++) // 修改变量名避免冲突
//                 {
//                     target.push_back(units[j]);
//                 }
//                 break;
//             }
//         }
//     }
//     if (!target.empty()) // 仅在非空时排序
//     {
//         sort(target.begin(), target.end());
//     }
//     // DEBUG_PRINT(target.size());
//     return target;
// }
vector<int> Scheduler::get_canceled_reqs_id()
{ // TODO 取消时 好像需要及时返回，每取消一个返回一个
    int threadNum = job_threads.size();
    vector<int> vec;
    vec.reserve(3 * threadNum);
    for (int i = 0; i < threadNum; i++)
    {
        PersuadeThread *t = &job_threads[i];
        //!!注意，没有考虑两磁盘有相同的取消请求；
        for (auto req : t->canceled_requests)
        {
            vec.push_back(req->id);
        }
    }
    return vec;
}
vector<int> Scheduler::get_complete_reqs_id()
{
    int threadNum = job_threads.size();
    vector<int> vec;
    vec.reserve(5 * threadNum); // 预留空间，这里直接硬编码，同时完成的任务应该不会很多，假设每个盘最多完成5个
    for (int i = 0; i < threadNum; i++)
    {
        PersuadeThread *t = &job_threads[i];
        for (auto req : t->complete_requests)
        {
            vec.push_back(req->id);
        }
    }
    return vec;
}
void Scheduler::excute_find()
{
    for(int i=0;i<job_threads.size();i++){
        job_threads[i].excute_find();
    }
}
void Scheduler::req_upload()
{
    Scheduler &SD = Scheduler::getInstance();
    DiskManager &DM = DiskManager::getInstance();
    lock_guard<std::mutex> lock(mutex_);
    // 统计完成请求数量和信息
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
    vector<int> complete_reqs = SD.get_complete_reqs_id();
    printf("%zd\n", complete_reqs.size());
    for (int req_id : complete_reqs)
    {
        info += to_string(req_id) + "\n";
    }
    printf("%s", info.c_str());
}

void Scheduler::end()
{
    for(auto &t:job_threads){
        t.end();
    }
}

unordered_map<int, std::vector<int>> &Scheduler ::get_active_requests()
{
    return active_requests;
}