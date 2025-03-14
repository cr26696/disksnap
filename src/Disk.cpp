// Disk.cpp
#include <iostream>
#include <cmath>

#include "Disk.hpp"
#include "Replica.hpp"
#include "MetaDefine.hpp"
#include "DiskManager.hpp"
using namespace std;
// 构造函数
Disk::Disk(int volume, int G,int id)
    : volume(volume), tokenG(G),id(id)
{
    head = 0;
    head_s = -1;
    elapsed = 0;
    blocks.resize(volume);
    job_count = 0;
    phase_end = false;
}

// 会移动head JUMP需传入跳转地址 PASS传入距离 READ无视参数可填0
// 返回操作是否成功 pass 0也算成功
bool Disk::operate(DiskOp op, int param)
{
    int consume_token = 0;

    switch (op)
    {
    case JUMP:
        DEBUG_LOG("ready jump");
        if (elapsed > 0)
        {
            phase_end = true;
            return false;
        }
        printf("j %d\n", param + 1);
        head = param % volume;
        head_s = 0;
        elapsed = tokenG + 1;
        phase_end = true;
        break;
    case PASS:
        if (param >= 1)
        {
            if (elapsed + param > tokenG)
            {
                phase_end = true;
                return false;
            }
            std::string s(param, 'p');
            printf("%s", s.c_str()); // 使用 c_str() 将 std::string 转换为 const char*
            head += param;
            head = head % volume;
            head_s = 1;
            elapsed += param;
            return true;
        }
        else if (param == 0)
            return true;
        return false;
        break;
    case READ:
        consume_token = 64;
        if (head_s > 16)
            consume_token = head_s;
        if (elapsed + consume_token >= tokenG)
        {
            phase_end = true;
            return false;
        }
        else
        {
            elapsed += consume_token;
            head_s = static_cast<int>(std::ceil(consume_token * 0.8));
            printf("r");
            head++;
        }
        break;
    default:
        break;
    }
    return true;
}
// 上报该磁盘磁头操作结束
void Disk::op_end()
{
    if (elapsed >= tokenG)
        return;

    printf("#\n");
    elapsed = tokenG;
    phase_end = true;
}
//取消对应的请求，记录在diskmanager 释放磁盘空间
void Disk::del_obj(int obj_id)
{
    unordered_set<int> canceled_reqs = DiskManager::getInstance().canceled_reqs;
    if (map_obj_replica.find(obj_id) == map_obj_replica.end())
    {
        throw std::invalid_argument("del_obj failed,No object id in this disk");
    }
    //  找到对应的副本
    Replica *rep = map_obj_replica[obj_id];
    // 取消的读取请求插入到向量 准备返回
    for(Request* req:map_obj_request[obj_id]){
        canceled_reqs.insert(req->id);
        job_count--;
    }
    map_obj_request[obj_id].clear(); // OPT 这里需不要从内存清除request？
    // 释放磁盘空间
    map_obj_part_addr.erase(obj_id);
    for (int part = 0; part < rep->size; part++)
    {
        blocks[part] = nullptr; //
    }
    // 从磁盘存储记录删除
    map_obj_replica.erase(obj_id);
    // 从内存中删除
    delete (rep);
}
void Disk::wrt_obj(Replica *replica)
{
    vector<Unit *> Units = replica->Units;
    for (int part = 0; part < Units.size(); part++)
    {
        // TODO 改成选最适合空间存放 大优化
        for (int i = 0; i < volume; i++)
        {
            if (blocks[i] == nullptr)
            {
                // 磁盘单元指向对应的 unit 可知道是第几块 对象id;
                blocks[i] = Units[part];
                map_obj_part_addr[replica->id][part] = i;
                break;
            }
        }
    }
    // 修改相关信息
    map_obj_replica[replica->id] = replica;
    if (map_obj_request.find(replica->id) == map_obj_request.end())
    {
        unordered_set<Request *> relative_req_set;
        map_obj_request[replica->id] = relative_req_set;
    }
}
// scheduler选定磁盘后，调用这个函数
void Disk::add_req(Request *req)
{
    // TODO
    job_count++;
    int obj_id = req->object_id;
    req->init_status(map_obj_replica[obj_id]->size);
    // 在wrt_obj时已确保set初始化
    unordered_set<Request *> &relative_reqs = map_obj_request[obj_id];
    relative_reqs.insert(req);
}
// 改到find逻辑
void Disk::task(std::vector<int> input_target, int disk_id)
{
    // DEBUG_PRINT(disk_id);
    // DEBUG_PRINT(head);
    // DEBUG_PRINT(elapsed);
    // if (input_target.empty())
    // {
    //     op_end();
    //     DEBUG_PRINT(input_target.size());
    //     return;
    // }

    // std::vector<int> target = input_target;
    // auto it = std::lower_bound(target.begin(), target.end(), head + 1);
    // if (it == target.begin() || it == target.end())
    // {
    //     DEBUG_LOG("neednt REarrage");
    // }
    // else
    // {
    //     std::vector<int> first_half(target.begin(), it);
    //     std::vector<int> second_half(it, target.end());
    //     target.clear();
    //     target.insert(target.end(), second_half.begin(), second_half.end());
    //     target.insert(target.end(), first_half.begin(), first_half.end());
    // }

    // int idx = 0;
    // int tokens = tokenG;
    // int distance = (target[0] - 1) - head;
    // vector<int> found;
    // if (distance < 0)
    //     distance += 8000;
    // DEBUG_PRINT(distance);
    // if (distance + 64 > tokens)
    // {
    //     operate(JUMP, target[0] - 1);
    //     op_end();
    //     return;
    // }
    // else
    // {
    //     while (elapsed < tokens)
    //     {
    //         if (distance > 0)
    //         {
    //             if (!operate(PASS, distance))
    //                 break;
    //         }
    //         if (!operate(READ, 0))
    //             break;
    //         found.push_back(target[idx]);
    //         idx++;
    //         if (idx >= target.size())
    //             break;
    //         distance = (target[idx] - 1) - head;
    //         if (distance < 0)
    //             distance += 8000;
    //     }
    // }
    // op_end();
    // vector<Request> &requests = RequestManager::getInstance()->getRequests();
    // vector<Object> &objects = ObjectManager::getInstance()->getObjects();
    // for (int i : found)
    // {
    //     int complete_id = 0;
    //     for (int j = 1; j <= 3; j++)
    //     {
    //         if (disk_id == objects[blocks[i]].replica[j])
    //         {
    //             for (int k = 1; k <= objects[blocks[i]].size; k++)
    //             {
    //                 if (objects[blocks[i]].unit[j][k] == i)
    //                 {
    //                     complete_id = k;
    //                     break;
    //                 }
    //             }
    //         }
    //         continue;
    //     }
    //     requests[objects[blocks[i]].last_request_point].complete[complete_id] = 1;
    // }
}
// 具体的查找算法，如果完成某个查找请求 放入compeletedTask
void Disk::find()
{
    // TODO
    // 完成的任务从map_obj_req 清楚 并将id添加到completed_reqs
}
// 刷新到下一帧 更新elapsed phase_end 清除完成的req
void Disk::end()
{
    // TODO
    elapsed = 0;
    phase_end = false;
    completed_reqs.clear();
}
