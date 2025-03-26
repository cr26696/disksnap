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

    for (int i = 0; i < tag_ratio.size(); i++)
    {
        // TODO 创建region
    }
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
    // TODO 判断并选用Region 调用Region的use_space方法
    // 根据将相关block写入
    int tagSpace = getRegionSpace(info.tag);
    if (tagSpace >= info.size)
    {
        // 直接存放
    }
    else
    {
        int region_idx = 0; // TODO 选择最适合的region
        for (int i = 0; i < regions.size(); i++)
        {
            if (i == info.tag)
                continue;
            if (regions[i].free_blocks > regions[region_idx].free_blocks)
                region_idx = i;
        }
        regions[region_idx].use_space(replicas[info.id]);
    }
}
void Disk::del_replica(Object &info)
{
    assert(replicas[info.id] != nullptr);
    int addr = replicas[info.id]->addr_part[0];
    int idx = 0;
    for (int i = 0; i < regions.size(); i++)
    {
        int start = regions[i].start;
        int end = regions[i].end;
        if (addr >= start && addr <= end)
        {
            idx = i;
            break;
        }
    }
    regions[idx].free_space(replicas[info.id]);
}
int Disk::getRegionSpace(int tag)
{
    return regions[tag].free_blocks;
}
int Disk::getAllSpace()
{
    int free_blocks = 0;
    for (int i = 0; i < regions.size(); i++)
    {
        free_blocks += regions[i].free_blocks;
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
