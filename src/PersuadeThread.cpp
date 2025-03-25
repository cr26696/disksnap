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
        // 未在目标上，尝试移动至目标
        if (disk->elapsed + distance + 64 > tokenG)
        {
            disk->operate(JUMP, *it_addr);
        }
        else
        {
            disk->operate(PASS, distance);
        }
    }
    disk->op_end();
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
