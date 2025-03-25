// Disk.cpp
#include <iostream>
#include <cmath>

#include "Disk.hpp"
#include "Replica.hpp"
#include "MetaDefine.hpp"
#include "DiskManager.hpp"
using namespace std;
// 构造函数
Disk::Disk(int volume, int G, int id, const std::vector<int>& label_index)
    : volume(volume), tokenG(G), id(id), free_capacity(volume)
{
    head = 0;
    head_s = -1;
    elapsed = 0;
    blocks.resize(volume);
    job_count = 0;
    phase_end = false;
    for(int i = 0; i < label_index.size() - 1; i++){
        subdisk_free_capacity.emplace_back(label_index[i + 1] - label_index[i]);
    }
    subdisk_free_capacity.emplace_back(volume - label_index.back());

    int start = 0;
    free_blocks.resize(label_index.size());
    for(int i = 0; i < label_index.size() - 1; i++){
        free_blocks[i].emplace_back(start, label_index[i+1] - 1);
        start = label_index[i + 1];
    }
    free_blocks[label_index.size() - 1].emplace_back(start, volume - 1);
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

/*
 * 删除本disk中编号为object_id的对象 并进行空闲块合并整理 按区间开头升序排序
 */
void Disk::delete_obj(int object_id)
{
    DiskManager &DM = DiskManager::getInstance();
    if (map_obj_replica.find(object_id) == map_obj_replica.end())
    {
        throw std::invalid_argument("del_obj failed,No object id in this disk");
    }
    Replica *replica = map_obj_replica[object_id]; // 找到对应的副本
    vector<int> temp_free_units;                   // 存放空闲碎片
    vector<pair<int, int>> temp_free_blocks;       // 存放合并空闲碎片后的空闲区间
    for (Request *req : map_obj_request[object_id])
    { // 存储将取消的读取请求
        DM.canceled_reqs.insert(req->id);
        job_count--;
    }
    map_obj_request[object_id].clear();
    /*----------开始释放磁盘----------*/
    for (int i : map_obj_part_addr[object_id])
    {
        assert(blocks[i] != nullptr);
        blocks[i] = nullptr;
        temp_free_units.emplace_back(i);
    }
    map_obj_replica.erase(object_id);
    free_capacity += replica->size;
    subdisk_free_capacity[replica->tag - 1] += replica->size;
    int deleted_tag = replica->tag - 1;
    delete (replica);
    /*----------开始整理空闲块----------*/
    sort(temp_free_units.begin(), temp_free_units.end()); // 将碎片块排序
    int start = temp_free_units[0], end = temp_free_units[0];
    for (int i = 1; i < temp_free_units.size(); i++)
    { // 将空闲碎片合并为空闲区间
        if (temp_free_units[i] == end + 1)
        {
            end = temp_free_units[i];
        }
        else if (temp_free_units[i] != end + 1)
        { // 若当前块与前一个块不连续
            temp_free_blocks.emplace_back(start, end);
            start = temp_free_units[i];
            end = temp_free_units[i];
        }
    }
    temp_free_blocks.emplace_back(start, end); // 插入最后一个区间 单块
    for (auto &block : temp_free_blocks)
    {
        free_blocks[deleted_tag].push_back(block);
    }
    free_blocks[deleted_tag].sort();
    for (auto current_block = free_blocks[deleted_tag].begin(); current_block != free_blocks[deleted_tag].end();)
    {
        auto next_block = next(current_block);
        if (next_block == free_blocks[deleted_tag].end())
            break;
        assert(current_block->second < next_block->first); // 理论上不会大于 最多相邻
        if (current_block->second == next_block->first - 1)
        { // 上一个空闲区别和下一个空闲区间相邻 需要进行合并
            current_block->second = next_block->second;
            free_blocks[deleted_tag].erase(next_block);
        }
        else
        {
            current_block++;
        }
    }
}

/*
 * 写入对象 优先满足连续存储 若无法连续存储 则进行分块存储
 */
void Disk::write_obj(Replica *replica, const std::vector<int>& label_index)
{
    int current_write_point = 0;
    int temp_write_point = 0;
    typedef list<pair<int, int>>::iterator p_it; // 记录暂存时所选择空闲块的迭代器
    vector<pair<p_it, int>> temp_operate;        // 记录暂存空闲块与块大小
    
    // 选择存储区域:自己tag区域能存就存，不能存就存到容量最大的tag区域
    int tag_area = replica->tag - 1;
    if(subdisk_free_capacity[replica->tag - 1] < replica->size){
        int temp_capacity = 0;
        for(int i = 0; i < label_index.size(); i++){
            if(subdisk_free_capacity[i] >= replica->size && temp_capacity < subdisk_free_capacity[i])
                tag_area = i;
                temp_capacity = subdisk_free_capacity[i];
        }   
    }    

    for (auto it = free_blocks[tag_area].begin(); it != free_blocks[tag_area].end(); it++)
    {
        int free_block_size = it->second - it->first + 1; // 当前空闲块的空间
        /*--------找到可连续存储块时 放弃分块暂存 直接存储--------*/
        if (free_block_size >= replica->size)
        {
            for (int i = 0; i < replica->size; i++)
            {
                assert(blocks[it->first + i] == nullptr);
                blocks[it->first + i] = &replica->Units[current_write_point]; // 磁盘单元指向对应的unit
                map_obj_part_addr[replica->id].push_back(it->first + i);      // 写入该副本的对象块在次磁盘中的位置
                current_write_point++;
            }
            if (free_block_size == replica->size)
            { // 若空闲块被填满，则删除该空闲块节点
                free_blocks[tag_area].erase(it);
            }
            else
            { // 若空闲块没有被填满，则修改该空闲块区间头
                it->first += replica->size;
            }
            assert(current_write_point == replica->size);
            // 修改相关信息
            map_obj_replica[replica->id] = replica;
            if (map_obj_request.find(replica->id) == map_obj_request.end())
            {
                vector<Request *> relative_req_set;
                map_obj_request[replica->id] = relative_req_set;
            }
            free_capacity -= replica->size;
            subdisk_free_capacity[tag_area] -= replica->size;
            return;
        }
        /*--------进行分块暂存记录 以防无法连续存储---------*/
        if (temp_write_point != replica->size)
        {
            int not_write_size = replica->size - temp_write_point; // 仍未存入的对象块大小
            // 标记暂存空间块使用情况 填满空闲块：填入剩余对象块剩余空间
            if (not_write_size >= free_block_size)
            {
                temp_operate.emplace_back(it, free_block_size);
                temp_write_point += free_block_size;
            }
            else
            {
                temp_operate.emplace_back(it, not_write_size);
                temp_write_point += not_write_size;
            }
        }
    }
    /*-------------使用暂存操作---------------*/
    for (auto it : temp_operate)
    {
        for (int i = 0; i < it.second; i++)
        {
            assert(blocks[it.first->first + i] == nullptr);
            blocks[it.first->first + i] = &replica->Units[current_write_point];
            // map_obj_part_addr[replica->id][current_write_point] = it.first->first + i;
            map_obj_part_addr[replica->id].push_back(it.first->first + i);
            current_write_point++;
        }
        int free_block_size = it.first->second - it.first->first + 1; // 当前空闲块的空间
        if (it.second == free_block_size)
        { // 如果填满了整个空闲块，则删除该空闲块
            free_blocks[tag_area].erase(it.first);
        }
        else
        { // 否则修改空闲块的起始位置
            it.first->first += it.second;
        }
    }
    // if(current_write_point != replica->size){
    //     printf("error: current_write_point != replica->size\n");
    //     printf("current_write_point: %d, replica->size: %d\n", current_write_point, replica->size);
    //     printf("replica->id: %d\n", replica->id);
    //     printf("replica->size: %d\n", replica->size);
    //     printf("replica->tag: %d\n", replica->tag);
    //     printf("map_obj_part_addr[replica->id]: ");
    //     for(int i = 0; i < map_obj_part_addr[replica->id].size(); i++){
    //         printf("%d ", map_obj_part_addr[replica->id][i]);
    //     }
    //     printf("map_obj_part_addr[replica->id].size(): %d\n", map_obj_part_addr[replica->id].size());
    //     printf("free_blocks[replica->tag - 1]: ");
    //     for(auto it = free_blocks[replica->tag - 1].begin(); it != free_blocks[replica->tag - 1].end(); it++){
    //         printf("(%d, %d) ", it->first, it->second);
    //     }
    //     printf("blocks.size(): %d\n", blocks.size());
    //     printf("free_capacity: %d\n", free_capacity);
    //     printf("subdisk_free_capacity[replica->tag - 1]: %d\n", subdisk_free_capacity[replica->tag - 1]);
    //     printf("\n");
    // }
    assert(current_write_point == replica->size);
    map_obj_replica[replica->id] = replica;
    if (map_obj_request.find(replica->id) == map_obj_request.end())
    {
        // unordered_set<Request *> relative_req_set;
        vector<Request *> relative_req_vector;
        map_obj_request[replica->id] = relative_req_vector;
    }
    free_capacity -= replica->size;
    subdisk_free_capacity[tag_area] -= replica->size;
    return;
}
// scheduler选定磁盘后，调用这个函数
// void Disk::add_req(Request *req)//
void Disk::add_req(int req_id, int obj_id)
{
    // OPT 将这个函数拆分到 伪线程 persuad_thread
    Request *req = new Request(req_id, obj_id, map_obj_replica[obj_id]->size,map_obj_part_addr[obj_id]);
    job_count++;
    vector<Request *> &relative_reqs = map_obj_request[obj_id];
    relative_reqs.push_back(req);
}

/*
 *计算每个磁盘的标签区域的剩余空间大小
 */
int Disk::numberOfFreeAllBlocks()
{
    return free_capacity;
}

int Disk::numberOfFreeSubBlocks(int tag){
    return subdisk_free_capacity[tag - 1];// 保留5块
}
// 具体的查找算法，如果完成某个查找请求 放入compeletedTask
void Disk::find()
{
    // OPT 磁盘内查找算法
    // 分析哪些块需要找
    // 根据块情况 操作磁头执行行动
    // 将找到的块记录 完成的请求从map_obj_req删除并将id添加到completed_reqs
    vector<int> target;
    int target_size = 1;
    // OPT 让没有请求的对应的objmap 为空
    for (const auto& [key, req_vec] : map_obj_request) {
        for (Request* req : req_vec) {
            target.insert(target.end(), req->tofind_addrs.begin(), req->tofind_addrs.end());
        }
        if(target.size() >= target_size) break;
    }
    // sort(target.begin(), target.end());
    // for (auto it = target.begin(); it != target.end(); ++it)
    // {
    //     if (*it >= head)
    //     {
    //         vector<int> first_half(target.begin(), it);
    //         vector<int> second_half(it, target.end());
    //         target.clear();
    //         target.insert(target.end(), second_half.begin(), second_half.end());
    //         target.insert(target.end(), first_half.begin(), first_half.end());
    //         break;
    //     }
    // }
    int idx = 0;
    while (idx != target.size() && !phase_end)
    {
        int distance = (target[idx] - head + volume) % volume; // 处理循环
        if (distance == 0)
        {
            if (operate(READ, 0))
            {
                // 先判断找到哪个块了
                Unit *unit = blocks[target[idx]];
                // 从需求中删除找到的块
                for (auto it_req = map_obj_request[unit->obj_id].begin(); it_req != map_obj_request[unit->obj_id].end();)
                {
                    Request *req = *it_req;
                    req->tofind_addrs.erase(remove(req->tofind_addrs.begin(), req->tofind_addrs.end(), target[idx]), req->tofind_addrs.end());
                    if (req->tofind_addrs.empty()) // 所有块找到了
                    {
                        completed_reqs.push_back(req->id);
                        it_req = map_obj_request[unit->obj_id].erase(it_req);
                        DiskManager::getInstance().completed_reqs.insert(req->id);
                    }
                    else
                        it_req++;
                }
            }
            if (++idx >= target.size())
                break; // 退出外层while循环
            distance = (target[idx] - head + volume) % volume;
            continue;
        }
        if (elapsed + distance + 64 > tokenG)
        {
            operate(JUMP, target[idx]);
            op_end();
        }
        else
        {
            operate(PASS, distance);
            operate(READ, 0);
        }
    }
    op_end();
}
// 刷新到下一帧 更新elapsed phase_end 清除完成的req
void Disk::end()
{
    elapsed = 0;
    phase_end = false;
    completed_reqs.clear();
}
