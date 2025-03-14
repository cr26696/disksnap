// Disk.cpp
#include <iostream>
#include <cmath>

#include "Disk.hpp"
#include "Replica.hpp"
#include "MetaDefine.hpp"
#include "DiskManager.hpp"
using namespace std;
// 构造函数
Disk::Disk(int volume, int G, int id)
    : volume(volume), tokenG(G), id(id)
{
    free_blocks.emplace_back(1, volume);
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
// 取消对应的请求，记录在diskmanager 释放磁盘空间
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
    for (Request *req : map_obj_request[obj_id])
    {
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
    for (int part = 0; part < replica->size; part++)
    {
        // TODO 改成选最适合空间存放 大优化
        for (int i = 0; i < volume; i++)
        {
            if (blocks[i] == nullptr)
            {
                // 磁盘单元指向对应的 unit 可知道是第几块 对象id;
                blocks[i] = &replica->Units[part];
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

/*
 * 删除对象 并进行空闲块合并整理 按区间开头升序排序
 */
void Disk::delete_obj(int *units, int object_size)
{
    vector<int> temp_free_units;             // 空闲碎片
    vector<pair<int, int>> temp_free_blocks; // 空闲区间
    for (int i = 1; i <= object_size; i++)
    {
        assert(blocks[units[i]] != 0);
        blocks[units[i]] = 0;
        temp_free_units.emplace_back(units[i]);
    }
    sort(temp_free_units.begin(), temp_free_units.end()); // 将碎片块排序
    int start = temp_free_units[0], end = temp_free_units[0];
    for (int i = 1; i < temp_free_units.size(); i++)
    { // 将空闲碎片合并为空闲区间
        if (temp_free_units[i] == end + 1)
        {
            end = temp_free_units[i];
        }
        else if (temp_free_units[i] != end + 1 || i == temp_free_units.size() - 1)
        { // 若当前块与前一个块不连续或为最后一个区间
            temp_free_blocks.emplace_back(start, end);
            start = temp_free_units[i];
            end = temp_free_units[i];
        }
    }
    for (auto &block : temp_free_blocks)
    {
        free_blocks.push_back(block);
    }
    free_blocks.sort();
    for (auto current_block = free_blocks.begin(); current_block != free_blocks.end();)
    {
        auto next_block = next(current_block);
        if (next_block == free_blocks.end())
            break;
        assert(current_block->second == next_block->first - 1); // 理论上不会大于
        if (current_block->second == next_block->first - 1)
        { // 表示上一个空闲区别和下一个空闲区间相近 需要进行合并
            current_block->second = next_block->second;
            free_blocks.erase(next_block);
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
void Disk::write_obj(int object_id, int *obj_units, int object_size)
{
    int current_write_point = 0;
    int temp_write_point = 0;
    typedef list<pair<int, int>>::iterator p_it; // 记录暂存时所选择空闲块的迭代器
    vector<pair<p_it, int>> temp_operate;        // 记录暂存空闲块与块大小
    for (auto it = free_blocks.begin(); it != free_blocks.end(); it++)
    {
        int free_block_size = it->second - it->first + 1; // 当前空闲块的空间
        // 找到可连续存储块时 放弃暂存 直接存储
        if (free_block_size >= object_size)
        {
            for (int i = 0; i < object_size; i++)
            { // 从0开始 因为可从首地址开始存储
                assert(blocks[it->first + i] == 0);
                blocks[it->first + i] = object_id;                // 存入磁盘
                obj_units[++current_write_point] = it->first + i; // 标记对象块位置
            }
            if (free_block_size == object_size)
            { // 若空闲块被填满，则删除该空闲块节点
                free_blocks.erase(it);
            }
            else
            { // 若空闲块没有被填满，则修改该空闲块区间头
                it->first += object_size;
            }
            assert(current_write_point == object_size);
            return;
        }
        // 进行分块暂存记录 以防无法连续存储
        if (temp_write_point != object_size)
        {
            int not_write_size = object_size - temp_write_point; // 仍未存入的对象块大小
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
    // 使用暂存操作
    for (auto it : temp_operate)
    {
        for (int i = 0; i < it.second; i++)
        {
            assert(blocks[it.first->first + i] == 0);
            blocks[it.first->first + i] = object_id;                // 存入磁盘
            obj_units[++current_write_point] = it.first->first + i; // 标记对象块位置
        }
        int free_block_size = it.first->second - it.first->first + 1;
        if (it.second == free_block_size)
        { // 如果填满了整个空闲块，则删除该空闲块
            free_blocks.erase(it.first);
        }
        else
        { // 否则修改空闲块的起始位置
            it.first->first += it.second;
        }
    }
    assert(current_write_point == object_size);
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

/*
 *计算总剩余空间大小
 */
int Disk::numberOfFreeBlocks_()
{
    int all_free_size = 0;
    for (auto it : free_blocks)
    {
        all_free_size += it.second - it.first + 1;
    }
    return all_free_size;
}

// 具体的查找算法，如果完成某个查找请求 放入compeletedTask
void Disk::find()
{
    // TODO
    // 分析哪些块需要找
    // 根据块情况 操作磁头执行行动
    // 将找到的块记录 完成的请求从map_obj_req删除并将id添加到completed_reqs
    vector<int> target;
    for (auto it_map = map_obj_request.begin(); it_map != map_obj_request.end(); ++it_map)
    {
        for (const Request *req : it_map->second)
        {
            target.insert(target.end(), req->needs.begin(), req->needs.end());
        }
    }
    sort(target.begin(), target.end());
    for (auto it = target.begin(); it != target.end(); ++it)
    {
        if (*it >= head)
        {
            vector<int> first_half(target.begin(), it);
            vector<int> second_half(it, target.end());
            target.clear();
            target.insert(target.end(), second_half.begin(), second_half.end());
            target.insert(target.end(), first_half.begin(), first_half.end());
            break;
        }
    }
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
                for (Request *req : map_obj_request[unit->obj_id])
                {
                    // 从需求中删除找到的块
                    req->needs.erase(remove(req->needs.begin(), req->needs.end(), target[idx]), req->needs.end());
                    if (req->needs.empty())
                    {
                        completed_reqs.push_back(req->id);
                        map_obj_request[unit->obj_id].erase(req);
                    }
                }
            }
            if (++idx >= target.size())
                break;//退出外层while循环
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
