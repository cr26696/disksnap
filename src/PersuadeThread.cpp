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
    task_requests.push_back(req);
    // 请求对应的块
    for (int i = 0; i < info.size; i++)
    {
        int addr = rep->addr_part[i];
        task_blocks.insert(addr);
    }
    // 不排序，添加到待找块中
}

void PersuadeThread::rmv_req(Object &obj)
{
    // 先执行此代码，需要确保磁盘存放了这个对象
    if (disk->get_replica(obj.id) == nullptr)
    {
        throw std::logic_error("PersuadeThread::rmv_req: try to get unexist object");
    }

    // 1. 删除对应任务
    for (auto it = task_requests.begin(); it != task_requests.end(); )
    {
        Request *req = *it;
        if (req->object_id == obj.id)
        {
            canceled_requests.push_back(req);
            it = task_requests.erase(it); // 使用迭代器删除
        }
        else
        {
            ++it; // 移动到下一个迭代器
        }
    }

    // 2. 删除对应块
    Replica *rep = disk->get_replica(obj.id);
    for (int i = 0; i < obj.size; i++)
    {
        int addr = rep->addr_part[i];
        task_blocks.erase(addr);
    }
}

// 按任务队列找
// void PersuadeThread::excute_find()
// {
//     // OPT 磁盘内查找算法
//     // 分析哪些块需要找
//     // 根据块情况 操作磁头执行行动
//     // 将找到的块记录 完成的请求从map_obj_req删除并将id添加到completed_reqs
//     // 处理空等情况
//     if (task_blocks.empty())
//     {
//         // 无事可做 直接结束
//         // OPT 多线程先暂存操作，之后按磁盘顺序返回
//         disk->op_end();
//         return;
//     }
//     auto it_addr = task_blocks.lower_bound(disk->head);
//     if (it_addr == task_blocks.end())
//     {
//         // 之前已处理为空的情况，这里是所有块地址都小于head
//         it_addr = task_blocks.begin();
//     }
//     int volume = disk->volume;
//     int tokenG = disk->tokenG;
//     int distance;
//     while (it_addr != task_blocks.end() && !disk->phase_end)
//     {
//         distance = (*it_addr - disk->head + volume) % volume; // 处理循环
//         // 已在目标上
//         if (distance == 0)
//         {
//             if (disk->operate(READ, 0))
//             {
//                 int obj_id, part;
//                 std::tie(obj_id, part) = disk->get_block(*it_addr);
//                 // 为所有需要此part的请求更新完成情况
//                 // 手动从uset构造出vec 方便遍历
//                 vector<Request *> task_vec;
//                 for (Request *req : task_requests)
//                 {
//                     task_vec.push_back(req);
//                 }
//                 // 遍历ve
//                 for (int i = 0; i < task_vec.size(); i++)
//                 {
//                     Request *req = task_vec[i];
//                     if (req->object_id == obj_id)
//                     {
//                         req->complete[part] = true;
//                         // 全部完成需要进行记录 并从task_request中删除
//                         if (req->is_complete())
//                         {
//                             complete_requests.push_back(req);
//                             task_requests.erase(req);
//                         }
//                     }
//                 }
//                 // 已找到块，从待找块地址task_blocks中删除
//                 it_addr = task_blocks.erase(it_addr);
//                 if (it_addr == task_blocks.end())
//                     break;
//                 distance = (*it_addr - disk->head + volume) % volume;
//                 continue; // 下次while循环
//             }
//             else
//             {
//                 // READ失败的情况token不足直接结束while循环
//                 break;
//             }
//         }
//         // 未在目标上，尝试移动至目标
//         if (disk->elapsed + distance + 64 > tokenG)
//         {
//             disk->operate(JUMP, *it_addr);
//         }
//         else
//         {
//             disk->operate(PASS, distance);
//         }
//     }
//     disk->op_end();
// }
                            // 修复后的代码
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

                // 手动从uset构造出vec 方便遍历
                vector<Request *> task_vec;
                task_vec.insert(task_vec.end(), task_requests.begin(), task_requests.end());
                // for (Request *req : task_requests)
                // {
                //     task_vec.push_back(req);
                // }

                // 遍历ve
                for (int i = 0; i < task_vec.size(); i++)
                {
                    Request *req = task_vec[i];
                    if (req->object_id == obj_id)
                    {
                        req->complete[part] = true;

                        // 全部完成需要进行记录 并从task_request中删除
                        if (req->is_complete())
                        {
                            complete_requests.push_back(req);

                            // 使用迭代器删除
                            auto it = std::find(task_requests.begin(), task_requests.end(), req);
                            if (it != task_requests.end())
                            {
                                task_requests.erase(it); // 修复点：传入迭代器
                            }
                        }
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
