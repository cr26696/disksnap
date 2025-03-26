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
    // 从第一个副本所在的磁盘线程开始比较
    for (int i = 1; i < REP_NUM; i++)
    {
        int compare_id = obj.diskid_replica[i];
        int current_size = job_threads[ideal_id].job_count;
        int compare_size = job_threads[compare_id].job_count;
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
void Scheduler::req_upload()
{
    Scheduler &SD = Scheduler::getInstance();
    DiskManager &DM = DiskManager::getInstance();
    lock_guard<std::mutex> lock(mutex_);
    // 统计完成请求数量和信息
    int complete_num = 0;
    string info = "";

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
    for (auto &t : job_threads)
    {
        t.end();
    }
}

unordered_map<int, std::vector<int>> &Scheduler ::get_active_requests()
{
    return active_requests;
}