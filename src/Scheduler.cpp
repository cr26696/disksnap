#include "System.hpp"
#include "Utils.hpp"
#include "Scheduler.hpp"
#include "DiskManager.hpp"
using namespace std;
Scheduler::Scheduler()
{
    DiskManager &DM = DiskManager::getInstance();
    int diskNum = DM.DiskNum;
    int last_check_time = 1;
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
    int min_score;
    for (int i = 0; i < REP_NUM; i++)
    {
        // 通义生成
        int disk_id = obj.diskid_replica[i];
        Disk *target_disk = &DiskManager::getInstance().get_disk(disk_id);
        Replica *rep = target_disk->get_replica(obj_id);

        // 准备数据计算综合评分
        vector<int> obj_addr;
        for (int j = 0; j < rep->info.size; j++)
            obj_addr.push_back(rep->addr_part[j]);
        int disk_head = target_disk->head;
        int volume = target_disk->volume;
        double score = rateReqToDisk(disk_head, obj_addr, job_threads[disk_id].job_center, job_threads[disk_id].job_count, volume);

        if (score > min_score)
        {
            ideal_id = disk_id;
            min_score = score;
        }
    }
    // 选好线程，调用添加任务函数
    Request *req = new Request(req_id, obj.id, obj.size, t);
    job_threads[ideal_id].add_req(req);

    return true; // 假设添加总是成功
}
// 对某个磁盘 某请求添加的得分 这个得分可以用于确定请求放在哪个磁盘
double Scheduler::rateReqToDisk(int disk_head, vector<int> &addrs, int job_center, int job_count, int volume)
{
    if (addrs.empty())
        throw range_error("add_request: addrs is empty");
    // 遍历每个磁盘，计算每个磁盘的评分
    double w_center = 0;
    double w_head = 0;
    double w_jobCount = 1;
    double score = 0.0;
    double center_score = 0.0;
    double head_score = 0.0;
    double job_load = 0.0;
    double job_delta = 0.5;
    for (int addr : addrs)
    {
        double d_center = minDistance(addr, job_center, volume); // 重心距离
        center_score += 1 - d_center * 2 / volume;
        double d_head = (addr - disk_head + volume) % volume;
        int tokenG = System::getInfo().TokenG;
        d_head = min(d_head, (double)tokenG);
        head_score += 1 - d_head / tokenG;
        job_load += job_delta;
        job_delta *= 0.5; // 级数 0.5 + 0.25 + 0.125
    }
    double job_score = 1 - job_load;
    score = center_score * w_center + head_score * w_head + job_score * w_jobCount;
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
    // int suspend_block_num = 0;
    // check_suspend(t);
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

void Scheduler::check_suspend(const int current_time)
{
    int suspend_block_num = 0;
    if (current_time - last_check_time >= CHECK_INTERVAL)
    {
        for (int i = 0; i < job_threads.size(); i++)
        {
            for (auto it_map = job_threads[i].map_obj_requests.begin(); it_map != job_threads[i].map_obj_requests.end() && suspend_block_num < MAX_SUSPEND_NUM; it_map++) // 进行task_blocks更新
            // for (auto it_map = job_threads[i].map_obj_requests.begin(); it_map != job_threads[i].map_obj_requests.end(); it_map++)
            {
                for (auto &req : it_map->second)
                {
                    req->suspend_request();
                    suspend_block_num += req->suspend_request();
                    if (suspend_block_num >= MAX_SUSPEND_NUM)
                        break;
                }
            }
        }
        last_check_time = current_time;
    }
}