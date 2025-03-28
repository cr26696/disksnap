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
    return job_threads[thread_index];
}

bool Scheduler::add_request(int req_id, int obj_id)
{
    // 选择最适合的disk放入任务
    // 找到能响应请求的disk
    DiskManager &DM = DiskManager::getInstance();
    Object &obj = DM.objects[obj_id];
    
    // OPT 选盘优化（可以加入工作强度选择磁盘->负载均衡）
    // 通过距离选择磁盘(存在分块且开始地址不一定最小)
    int ideal_id = obj.diskid_replica[0];
    int min_distance;
    for (int i = 0; i < REP_NUM; i++)
    {
        //通义生成
        int disk_id = obj.diskid_replica[i];
        Disk* target_disk = &DiskManager::getInstance().get_disk(disk_id);
        Replica* rep = target_disk->get_replica(obj_id);
        
        // 计算对象在该磁盘上的起始位置（取第一个块地址）
        int obj_start_addr = rep->addr_part[0];   //第一个不一定储存在最前面...
        int disk_head = target_disk->head;
        int volume = target_disk->volume;
        int distance = (obj_start_addr - disk_head + volume) % volume;
        
        if (i == 0){
            min_distance = distance;
            continue;
        }    
        if (distance < min_distance) 
        {
            ideal_id = disk_id;
            min_distance = distance;
        }
    }
    // 选好线程，调用添加任务函数
    job_threads[ideal_id].add_req(req_id, obj);

    return true; // 假设添加总是成功
}

vector<int> Scheduler::get_canceled_reqs_id()
{
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
    for (int i = 0; i < job_threads.size(); i++)
    {
        job_threads[i].excute_find();
    }
}
string Scheduler::getUploadInfo()
{
    string info = "";
    Scheduler &SD = Scheduler::getInstance();
    // 统计完成请求数量和信息
    int complete_num = 0;
    vector<int> complete_reqs = SD.get_complete_reqs_id();
    info += to_string(complete_reqs.size()) + "\n";
    for (int req_id : complete_reqs)
    {
        info += to_string(req_id) + "\n";
    }
    return info;
}

void Scheduler::end()
{
    for (auto &t : job_threads)
    {
        t.end();
    }
}

unordered_map<int, std::vector<int>> &Scheduler ::get_active_requests()
{
    return active_requests;
}