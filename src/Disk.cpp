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
    free_blocks.emplace_back(0, volume-1);
    head = 0;
    head_s = -1;
    elapsed = 0;
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
            return true;
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
void Disk::del_obj(Object &info)
{
    int object_id = info.id;
    int size = info.size;
    // unordered_set<int> canceled_reqs = DiskManager::getInstance().canceled_reqs;
    if (replicas[object_id] == nullptr)
    {
        throw std::invalid_argument("del_obj failed,No object id in this disk");
    }
    Replica *replica = replicas[object_id]; // 找到对应的副本
    vector<int> temp_free_units;                   // 存放空闲碎片
    vector<pair<int, int>> temp_free_blocks;       // 存放合并空闲碎片后的空闲区间
    /*----------开始释放磁盘----------*/
    for (int i=0;i<size;i++)
    {
        int addr = replica->addr_part[i];
        // assert(blocks[i] != nullptr);
        blocks[addr].reset();
        temp_free_units.emplace_back(addr);
    }
    replicas[object_id] = nullptr;
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
        free_blocks.push_back(block);
    }
    free_blocks.sort();
    for (auto current_block = free_blocks.begin(); current_block != free_blocks.end();)
    {
        auto next_block = next(current_block);
        if (next_block == free_blocks.end())
            break;
        assert(current_block->second < next_block->first); // 理论上不会大于 最多相邻
        if (current_block->second == next_block->first - 1)
        { // 上一个空闲区别和下一个空闲区间相邻 需要进行合并
            current_block->second = next_block->second;
            free_blocks.erase(next_block);
        }
        else
        {
            current_block++;
        }
    }
}
void Disk::wrt_obj(Object &info)
{
    int obj_id = info.id;
    int size = info.size;
    // 直接尝试写入一个新的 对象
    replicas[obj_id] = new Replica{info};//初始化 注意part中全为0，使用需要根据size判断
    Replica* replica = replicas[obj_id];
    int current_write_point = 0;
    int temp_write_point = 0;
    typedef list<pair<int, int>>::iterator p_it; // 记录暂存时所选择空闲块的迭代器
    vector<pair<p_it, int>> temp_operate;        // 记录暂存空闲块与块大小
    for (auto it = free_blocks.begin(); it != free_blocks.end(); it++)
    {
        int free_block_size = it->second - it->first + 1; // 当前空闲块的空间
        /*--------找到可连续存储块时 放弃分块暂存 直接存储--------*/
        if (free_block_size >= size)
        {
            for (int i = 0; i < size; i++)
            {
                // assert(blocks[it->first + i] == nullptr);
                blocks[it->first + i] = std::make_pair(obj_id,i);        // 磁盘单元指向对应的unit
                replica->addr_part[current_write_point]=it->first+i;
                // map_obj_part_addr[replica->id][current_write_point] = it->first + i; // 写入该副本的对象块在次磁盘中的位置
                current_write_point++;
            }
            if (free_block_size == size)
            { // 若空闲块被填满，则删除该空闲块节点
                free_blocks.erase(it);
            }
            else
            { // 若空闲块没有被填满，则修改该空闲块区间头
                it->first += size;
            }
            assert(current_write_point == size);
            return;
        }
        /*--------进行分块暂存记录 以防无法连续存储---------*/
        if (temp_write_point != size)
        {
            int not_write_size = size - temp_write_point; // 仍未存入的对象块大小
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
            // assert(blocks[it.first->first + i] == nullptr);
            blocks[it.first->first + i] = std::make_pair(obj_id,current_write_point);
            replica->addr_part[current_write_point]=it.first->first + i;
            // map_obj_part_addr[replica->id][current_write_point] = it.first->first + i;
            current_write_point++;
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
    assert(current_write_point == size);
    return;
}
std::pair<int, int> Disk::get_block(int addr)
{
	return blocks[addr].value();//为空直接报错
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
//调用者自行判断对象指针是否为nullptr(不存在或被删除)
Replica *Disk::get_replica(int obj_id)
{
	return replicas[obj_id];
}

vector<int> Disk::get_store_pos(int obj_id)
{
    Replica* rep = replicas[obj_id];
    int size = rep->info.size;
    vector<int> addrs;
    for(int i=0;i<size;i++){
        addrs.push_back(rep->addr_part[i]);
    }
	return addrs;
}

// 刷新到下一帧 更新elapsed phase_end 清除完成的req
void Disk::end()
{
    elapsed = 0;
    phase_end = false;
}
