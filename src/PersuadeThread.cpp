#include "PersuadeThread.hpp"
#include "DiskManager.hpp"

using namespace std;

PersuadeThread::PersuadeThread(Disk *disk_ptr) : disk(disk_ptr)
{
}

// 创建任务 添加到活跃任务
void PersuadeThread::add_req(int req_id, Object &info)
{
    Request *req = new Request(req_id, info.id, info.size);
    Replica *rep = disk->get_replica(info.id);
    // req->init_status(map_obj_replica[obj_id]->size);
    // 在wrt_obj时已确保set初始化
    // unordered_set<Request *> &relative_reqs = map_obj_request[obj_id];
    // relative_reqs.insert(req);
    // 请求
    // task_requests.push_back(req);
    // 请求对应的块
    for (int i = 0; i < info.size; i++)
    {
        int addr = rep->addr_part[i];
        task_blocks.insert(addr);
    }
    map_obj_requests[info.id].push_back(req);
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

    vector<Request*> req_vec = map_obj_requests[obj.id];
    for(Request* req : req_vec){
        canceled_requests.push_back(req);
        job_count--;
    }
    
    // 2. 删除对应块
    Replica *rep = disk->get_replica(obj.id);
    for (int i = 0; i < obj.size; i++)
    {
        int addr = rep->addr_part[i];
        task_blocks.erase(addr);
    }
    map_obj_requests.erase(obj.id);
}

// 按任务队列找
void PersuadeThread::excute_find()
{
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
                int obj_id, part;
                std::tie(obj_id, part) = disk->get_block(*it_addr);

                /* 这个对象有请求 */
                // if(map_obj_requests.find(obj_id) != map_obj_requests.end())
                // {
                    // int part = disk->blocks[*it_addr].value().second;
                    assert(map_obj_requests.find(obj_id) != map_obj_requests.end());
                    for(auto req = map_obj_requests[obj_id].begin(); req != map_obj_requests[obj_id].end();)
                    {
                        (*req)->complete[part] = true;
                        if ((*req)->is_complete())//TODO 改为全部找完进行判定上报
                        {
                            complete_requests.push_back(*req);
                            job_count--;
                            req = map_obj_requests[obj_id].erase(req);
                        }
                        else
                        {
                            ++req;
                        }
                    }
                // }
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
            //环形判断
            if (it == task_blocks.end())
                it = task_blocks.begin();
        }
        // 未在目标上，尝试移动至目标(距离过长jump，距离中等pass，距离短判断使用pass/read)
        if (disk->elapsed + distance + 64 > tokenG)    //这里jump没问题吗？
            disk->operate(JUMP, *it_addr);
        else
        {   
            if (distance <= 4){
                int pass_cus = distance + read_custom(64, task_len);
                int read_cus = read_custom(disk->head_s, distance + task_len);
                if(pass_cus >= read_cus)
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

//计算读取消耗的token
int PersuadeThread::read_custom(int current_custom,int len){
    int read_custom = 0;
    
    for(int i = 0; i < len; i++){
        if (current_custom == 16)
            read_custom += current_custom;
        else if (current_custom == -1 || current_custom == 0){
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
