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
    //构造磁盘tag区域
    int region_start = 0;
    for (int i = 0; i < tag_ratio.size() - 1; i++)
    {
        int region_size = static_cast<int>(tag_ratio[i] * volume);
        int region_end = region_start + region_size - 1;
        regions.push_back(DiskRegion(region_start, region_end));
        blocks[region_start].start = true;
        blocks[region_end].end = true;
        region_start = region_end + 1;
    }
    regions.push_back(DiskRegion(region_start, volume - 1));
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
//返回（空格+块写入地址）* size
string Disk::wrt_replica(Object &info)
{
    // 判断并选用Region，调用Region的use_space方法
    replicas[info.id] = new Replica(info);
    Replica *replica = replicas[info.id];
    vector<int> addrs;
    int idx_tag = info.tag -1; // tag从1开始 作索引使用时-1
    if (getRegionSpace(idx_tag) >= info.size)
        addrs = regions[idx_tag].use_space(replica);
    else
    {
        int region_idx = 0;
        for (int i = 1; i < regions.size(); i++)
        {
            if (i == idx_tag)
                continue;
            if (regions[i].free_blocks_size > regions[region_idx].free_blocks_size)
                region_idx = i;
        }
        addrs = regions[region_idx].use_space(replica);
    }
    string s = to_string(id+1);//盘号（注意从1开始) + 对象各块存储位置
    for(int i = 0; i < info.size; i++){
        int addr = addrs[i];
        blocks[addr].used = true;
        blocks[addr].obj_id = info.id;
        blocks[addr].part = i;
        replica->addr_part[i] = addr;
        s += " " + to_string(addr+1);//空格 + 块存入地址（从1开始）
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
    regions[idx_region].free_space(replica);
    delete(replica);
}
int Disk::getRegionSpace(int tag)
{
    return regions[tag-1].free_blocks_size;
}
int Disk::getAllSpace()
{
    int free_blocks = 0;
    for (int i = 0; i < regions.size(); i++)
    {
        free_blocks += regions[i].free_blocks_size;
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
}

int Disk::get_regionIndix(int addr)
{
    for (int i = 0; i < regions.size(); i++)
    {
        int start = regions[i].start;
        int end = regions[i].end;
        if (addr >= start && addr <= end)
        {
            return i;
        }
    }
    assert(true);// should not reach here
    return -1;
}