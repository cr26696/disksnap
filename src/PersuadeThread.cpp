#include "PersuadeThread.hpp"
#include "DiskManager.hpp"
#include <cmath>

using namespace std;

PersuadeThread::PersuadeThread(Disk *disk_ptr) : disk(disk_ptr)
{
    last_get_time = 1;
}

// 创建任务 添加到活跃任务
void PersuadeThread::add_req(Request *req)
{
    Replica *replica = disk->get_replica(req->object_id);
    for (int i = 0; i < req->size; i++)
    {
        int addr = replica->addr_part[i];
        req->req_units[i].addr = addr;
        req->req_units[i].pDisk = disk;
        updata_job_center(true, addr);
        // task_blocks.insert(addr);
    }
    map_obj_requests[req->object_id].push_back(req);
    job_count++;
    // 不排序，添加到待找块中
}

void PersuadeThread::rmv_req(Object &obj)
{
    // 先执行此代码，需要确保磁盘存放了这个对象
    if (disk->get_replica(obj.id) == nullptr)
    {
        throw std::logic_error("PersuadeThread::rmv_req: try to get unexist object");
    }

    vector<Request *> req_vec = map_obj_requests[obj.id];
    for (Request *req : req_vec)
    {
        canceled_requests.push_back(req);
        job_count--;
    }

    // 2. 删除对应块
    if (PRINT_SROCE)
    {
        string new_content;
        ofstream file("sorce_info.txt", ios::app);
        for (Request *req : canceled_requests)
        {
            new_content = "no" + to_string(req->id) + "(" + to_string(req->object_id + 1) + ")" + " ";
            for (int i = 0; i < req->size; i++)
            {
                new_content += to_string(req->req_units[i].addr) + "(" + to_string(req->req_units[i].find_time) + ") ";
            }
            new_content += to_string(req->add_time) + "~" + to_string(t) + "\n";
            file << new_content;
        }
    }
    Replica *rep = disk->get_replica(obj.id);
    for (int i = 0; i < obj.size; i++)
    {
        int addr = rep->addr_part[i];
        updata_job_center(false, addr);
        task_blocks.erase(addr);
    }
    map_obj_requests.erase(obj.id);
}

void PersuadeThread::updata_job_center(bool is_add, int addr)
{
    int volume = disk->volume;
    int len_ni = ((int)round(job_center) - addr + volume) % volume;
    int len_shun = (addr - (int)round(job_center) + volume) % volume;
    int len_addr = len_ni <= len_shun ? job_center - len_ni : job_center + len_shun;
    if (is_add)
        job_center = (task_blocks.size() * job_center + len_addr) / (task_blocks.size() + 1);
    else
        job_center = (task_blocks.size() * job_center - len_addr) / (task_blocks.size() - 1);
    job_center = fmod((job_center + volume), volume);
}

// 按任务队列找
void PersuadeThread::excute_find()
{
    get_task_blocks(t);
    // OPT 磁盘内查找算法
    if (task_blocks.empty())
    {
        disk->op_end();
        return;
    }
    auto it_addr = task_blocks.lower_bound(disk->head);
    if (it_addr == task_blocks.end())
    {
        it_addr = task_blocks.begin();
    }
    int volume = disk->volume;
    int tokenG = disk->tokenG;
    int distance;
    while (it_addr != task_blocks.end() && !disk->phase_end)
    {
        distance = (*it_addr - disk->head + volume) % volume;
        if (distance == 0)
        {
            if (disk->operate(READ, 0))
            {
                // int reg_idx = disk->get_regionIndix(*it_addr);
                int obj_id = disk->blocks[*it_addr].obj_id;
                int part = disk->blocks[*it_addr].part;
                /* 这个对象有请求 */
                assert(map_obj_requests.find(obj_id) != map_obj_requests.end());
                for (auto req = map_obj_requests[obj_id].begin(); req != map_obj_requests[obj_id].end();)
                {
                    (*req)->req_units[part].complete = true;
                    (*req)->req_units[part].find_time = t; // 记录当前part完成时间
                    // (*req)->dnot_comp_num--;
                    // assert((*req)->dnot_comp_num >= 0);
                    if ((*req)->is_complete())
                    {
                        if (PRINT_SROCE)
                        {
                            string new_content;
                            ofstream file("sorce_info.txt", ios::app);
                            new_content = "ok" + to_string((*req)->id) + "(" + to_string((*req)->object_id + 1) + ")" + " ";
                            for (int i = 0; i < (*req)->size; i++)
                            {
                                new_content += to_string((*req)->req_units[i].addr) + "(" + to_string((*req)->req_units[i].find_time) + ") ";
                            }
                            new_content += to_string((*req)->add_time) + "~" + to_string(t) + " disk-" + to_string(disk->id + 1) + "\n";
                            file << new_content;
                        }
                        complete_requests.push_back(*req);
                        (*req)->req_complete_time = t; // 记录请求完成时间
                        job_count--;
                        req = map_obj_requests[obj_id].erase(req);
                    }
                    else
                    {
                        ++req;
                    }
                }
                // 已找到块，从待找块地址task_blocks中删除
                it_addr = task_blocks.erase(it_addr);
                if (it_addr == task_blocks.end())
                    break;
                distance = (*it_addr - disk->head + volume) % volume;
                continue; // 下次while循环
            }
            else
            {
                break;
            }
        }
        // 计算后续需要读取的区域长度 task_len，通义生成
        int task_len = 0;
        auto it = it_addr;
        int current_addr = *it;
        while (*it == current_addr)
        {
            task_len++;
            current_addr = (current_addr + 1) % volume;
            ++it;
            // 环形判断
            if (it == task_blocks.end())
                it = task_blocks.begin();
        }
        // 未在目标上，尝试移动至目标(距离过长jump，距离中等pass，距离短判断使用pass/read)
        if (disk->elapsed + distance + 64 > tokenG) // 这里jump没问题吗？
            disk->operate(JUMP, *it_addr);
        else
        {
            if (distance <= 4)
            {
                int pass_cus = distance + read_custom(64, task_len);
                int read_cus = read_custom(disk->head_s, distance + task_len);
                if (pass_cus >= read_cus)
                    disk->operate(READ, 0);
                else
                    disk->operate(PASS, distance);
            }
            else
                disk->operate(PASS, distance);
        }
    }
    disk->op_end();
}

// 计算读取消耗的token
int PersuadeThread::read_custom(int current_custom, int len)
{
    int read_custom = 0;

    for (int i = 0; i < len; i++)
    {
        if (current_custom == 16)
            read_custom += current_custom;
        else if (current_custom == -1 || current_custom == 0)
        {
            current_custom = 64;
            read_custom += current_custom;
        }
        else
        {
            current_custom = static_cast<int>(std::ceil(current_custom * 0.8));
            read_custom += current_custom;
        }
    }
    return read_custom;
}

void PersuadeThread::get_task_blocks(const int current_time)
{
    if (current_time - last_get_time > CHECK_TAKS_TIME)
    {
        struct Compare
        {
            bool operator()(const std::pair<double, Request *> &a, const std::pair<double, Request *> &b)
            {
                return a.first < b.first; // 按 first 的降序排列（最大堆）
            }
        };
        priority_queue<pair<double, Request *>, vector<pair<double, Request *>>, Compare> req_val_heap;
        vector<Request *> reqs_vec;
        for (auto it_map = map_obj_requests.begin(); it_map != map_obj_requests.end(); ++it_map)
        {
            for (Request *req : it_map->second)
            {
                // assert(req->dnot_comp_num > 0);
                double req_val = 0.0;
                double req_sorce = req->get_sorce(t);
                int same_ojb_req_num = it_map->second.size();
                int do_not_comp_rate = req->do_not_complete_num() * req->do_not_complete_num() * req->do_not_complete_num();
                if (do_not_comp_rate != 0)
                {
                    req_val = req_sorce * same_ojb_req_num / do_not_comp_rate;
                }
                req_val_heap.push(make_pair(req_val, req));
            }
        }
        int count = req_val_heap.size() * MAX_PROCESS_RATE;
        task_blocks.clear();
        while (count--)
        {
            Request *req = req_val_heap.top().second;
            for (int i = 0; i < req->size; i++)
            {
                task_blocks.insert(req->req_units[i].addr);
            }
            req_val_heap.pop();
        }
        last_get_time = current_time;
    }
}

void PersuadeThread::end()
{
    // 注意回收内存
    if (!canceled_requests.empty())
    {
        for (int i = 0; i < canceled_requests.size(); i++)
        {
            delete (canceled_requests[i]);
        }
        canceled_requests.clear();
    }
    if (!complete_requests.empty())
    {
        for (int i = 0; i < complete_requests.size(); i++)
        {
            delete (complete_requests[i]);
        }
        complete_requests.clear();
    }
}
