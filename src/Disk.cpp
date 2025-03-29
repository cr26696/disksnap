// Disk.cpp
#include <iostream>
#include <cmath>

#include "Disk.hpp"
#include "Replica.hpp"
#include "MetaDefine.hpp"
#include "DiskManager.hpp"
using namespace std;
// 构造函数
Disk::Disk(int volume, int G, int id, vector<double> &tag_ratio)
    : volume(volume), tokenG(G), id(id)
{
    head = 0;
    head_s = -1;
    elapsed = 0;
    phase_end = false;
    freeBlocks = 0;
    // 构造磁盘tag区域
    int region_start = 0;
    for (int i = 0; i < tag_ratio.size() - 1; i++)
    {
        int region_size = static_cast<int>(tag_ratio[i] * volume);
        int region_end = region_start + region_size - 1;
        regions.push_back(DiskRegion(region_start, region_end, i + 1, *this));
        blocks[region_start].start = true;
        blocks[region_end].end = true;
        region_start = region_end + 1;
    }
    regions.push_back(DiskRegion(region_start, volume - 1, tag_ratio.size(), *this));
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
        // printf("j %d\n", param + 1);
        upload_info += "j " + to_string(param + 1) + "\n";
        head = param % volume;
        head_s = 0;
        elapsed = tokenG + 1;
        phase_end = true;
        break;
    case PASS: // SOPT 根据标签读取热值判断走or不走
        if (param >= 1)
        {
            if (elapsed + param > tokenG)
            {
                param = tokenG - elapsed;
                string s(param, 'p');
                upload_info.append(s);
                head += param;
                head = head % volume;
                head_s = 1;
                elapsed += param;
                phase_end = true;
                return true;
            }
            string s(param, 'p');
            upload_info.append(s);
            // printf("%s", s.c_str()); // 使用 c_str() 将 std::string 转换为 const char*
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
            upload_info += "r";
            // printf("r");
            head++;
            head = head % volume;
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
    upload_info += "#\n";
    // printf("#\n");
    elapsed = tokenG;
    phase_end = true;
}
// 返回（空格+块写入地址）* size
string Disk::wrt_replica(Object &info)
{
    // 判断并选用Region，调用Region的use_space方法
    replicas[info.id] = new Replica(info);
    Replica *replica = replicas[info.id];
    vector<int> addrs;
    int idx_tag = info.tag - 1; // tag从1开始 作索引使用时-1
    if (regions[idx_tag].space_count >= info.size)
        addrs = regions[idx_tag].use_space(info.size);
    else
    {
        vector<int> region_indexs;
        region_indexs.reserve(regions.size());
        for (int i = 0; i < regions.size(); i++)
            region_indexs.push_back(i);
        auto compare_tag_distance = [idx_tag](int a, int b)
        {
            int dist_a = min(abs(idx_tag - a), abs(a - idx_tag));
            int dist_b = min(abs(idx_tag - b), abs(b - idx_tag));
            return dist_a < dist_b;
        };
        sort(region_indexs.begin(), region_indexs.end(), compare_tag_distance);
        bool can_store = false;
        for (auto region_index : region_indexs)
        {
            if (regions[region_index].space_count > info.size)
            {
                addrs = regions[region_index].use_space(info.size);
                can_store = true;
                break;
            }
        }
        if (!can_store)
            throw logic_error("no region can store obj " +
                              to_string(info.id) + " of size " +
                              to_string(info.size) + " in disk " +
                              to_string(id));
    }
    string s = to_string(id + 1); // 盘号（注意从1开始) + 对象各块存储位置
    for (int i = 0; i < info.size; i++)
    {
        int addr = addrs[i];
        blocks[addr].used = true;
        blocks[addr].obj_id = info.id;
        blocks[addr].part = i;
        replica->addr_part[i] = addr;
        s += " " + to_string(addr + 1); // 空格 + 块存入地址（从1开始）
    }
    return s;
}
void Disk::del_replica(Object &info)
{
    assert(replicas[info.id] != nullptr);
    Replica *replica = replicas[info.id];
    int part1_addr = replicas[info.id]->addr_part[0];
    int idx_region = get_regionIndix(part1_addr);
    replicas[info.id] = nullptr;
    vector<int> addrs;
    addrs.reserve(info.size);
    for (int i = 0; i < info.size; i++)
    {
        addrs.push_back(replica->addr_part[i]);
        blocks[replica->addr_part[i]].used = false;
        blocks[replica->addr_part[i]].obj_id = -1;
        blocks[replica->addr_part[i]].part = -1;
    }
    regions[idx_region].free_space(addrs);
    delete (replica);
}
//!! 这里一定要填tag 从1开始的tag 而不是转换的索引
int Disk::getRegionSpace(int tag)
{
    return regions[tag - 1].space_count;
}
int Disk::getAllSpace()
{
    int free_blocks = 0;
    for (int i = 0; i < regions.size(); i++)
    {
        free_blocks += regions[i].space_count;
    }
    return free_blocks;
}
// 调用者自行判断对象指针是否为nullptr(不存在或被删除)
Replica *Disk::get_replica(int obj_id)
{
    return replicas[obj_id];
}
// 刷新到下一帧 更新elapsed phase_end 清除完成的req
void Disk::end()
{
    elapsed = 0;
    phase_end = false;
    upload_info.clear();
}

int Disk::get_regionIndix(int addr)
{
    for (int i = 0; i < regions.size(); i++)
    {
        int start = regions[i].region_start;
        int end = regions[i].region_end;
        if (addr >= start && addr <= end)
        {
            return i;
        }
    }
    assert(true); // should not reach here
    return -1;
}