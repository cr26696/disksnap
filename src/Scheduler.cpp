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
    
    int ideal_id = obj.diskid_replica[0];
    int min_score = 0;
    for (int i = 0; i < REP_NUM; i++)
    {
        int disk_id = obj.diskid_replica[i];
        Disk* target_disk = &DiskManager::getInstance().get_disk(disk_id);
        Replica* rep = target_disk->get_replica(obj_id);
        
        //准备数据计算综合评分
        vector<int> obj_addr;   
        for (int j = 0; j < rep->info.size; j++)
            obj_addr.push_back(rep->addr_part[j]);
        int disk_head = target_disk->head;
        int volume = target_disk->volume;
        double score = rating_disk_requests(disk_head, obj_addr, job_threads[disk_id].job_center, job_threads[disk_id].job_count, volume);
   
        if (score > min_score) 
        {
            ideal_id = disk_id;
            min_score = score;
        }
    }
    // 选好线程，调用添加任务函数
    Request *req = new Request(req_id, obj.id, obj.size);
    job_threads[ideal_id].add_req(req);

    return true; // 假设添加总是成功
}
double Scheduler::rating_disk_requests(int disk_head, vector<int> &addrs, int job_center,int job_count, int volume)
{
    // 遍历每个磁盘，计算每个磁盘的评分
    vector<double> score_weight = {4, 1, 5};
    double score = 0.0;
    double total_center = 0.0;
    double total_distance = 0.0;
    for (int addr : addrs){
        total_center += (addr - job_center + volume) % volume; //重心距离
        total_distance += (addr - disk_head + volume) % volume; //磁头距离
    }
    double center_score = total_center / addrs.size() / volume;
    double head_score = total_distance / addrs.size() / volume;
    if(job_count < 100)
        score_weight = {1, 4, 5};
    score = (1-center_score)*score_weight[0] + (1-head_score)*score_weight[1] + (1/(job_count + 1))*score_weight[2];
    
    return score;
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

double Scheduler::job_rating(vector<int> addrs,int job_center)
{
    //TODO 评分函数
    //去拿各个盘副本 拿到各块地址
    //考虑对象各块分散度
    //与3重心距离，2磁头前后，1任务负载的分段函数） 得一评分 任务下发依据这个评分
	return 0.0;
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