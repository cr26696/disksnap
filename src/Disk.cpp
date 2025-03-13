// Disk.cpp
#include "Disk.hpp"
using namespace std;
// 构造函数
Disk::Disk(int V, int G)
    : head(0), head_s(-1), sizeV(V), tokenG(G), elapsed(0), phase_end(false), blocks(V, 0)
{

}

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
        head = param % sizeV;
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
            head = head % sizeV;
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

void Disk::op_end()
{
    if (elapsed == tokenG + 1)
    {
        elapsed = 0;
        phase_end = false;
        return;
    }
    printf("#\n");
    elapsed = 0;
    phase_end = false;
}

// void Disk::init(int G, int V)
// {
//     head = 0;
//     token = G;
//     size = V;
// }

void Disk::delete_obj(int *units, int size)
{
    for (int i = 1; i <= size; i++)
    {
        blocks[units[i]] = 0;
    }
}
//写入一个副本
//往map中插入
void Disk::write_obj(int object_id, int *obj_units, int size)
{
    int current_write_point = 0;
    for (int i = 1; i <= size; i++)
    {
        if (blocks[i] == 0)
        {
            blocks[i] = object_id;
            obj_units[++current_write_point] = i;
            if (current_write_point == size)
            {
                break;
            }
        }
    }
    assert(current_write_point == size);
}

void Disk::task(std::vector<int> input_target, int disk_id)
{
    DEBUG_PRINT(disk_id);
    DEBUG_PRINT(head);
    DEBUG_PRINT(elapsed);
    if (input_target.empty())
    {
        op_end();
        DEBUG_PRINT(input_target.size());
        return;
    }

    std::vector<int> target = input_target;
    auto it = std::lower_bound(target.begin(), target.end(), head + 1);
    if (it == target.begin() || it == target.end())
    {
        DEBUG_LOG("neednt REarrage");
    }
    else
    {
        std::vector<int> first_half(target.begin(), it);
        std::vector<int> second_half(it, target.end());
        target.clear();
        target.insert(target.end(), second_half.begin(), second_half.end());
        target.insert(target.end(), first_half.begin(), first_half.end());
    }

    int idx = 0;
    int tokens = tokenG;
    int distance = (target[0] - 1) - head;
    vector<int> found;
    if (distance < 0)
        distance += 8000;
    DEBUG_PRINT(distance);
    if (distance + 64 > tokens)
    {
        operate(JUMP, target[0] - 1);
        op_end();
        return;
    }
    else
    {
        while (elapsed < tokens)
        {
            if (distance > 0)
            {
                if (!operate(PASS, distance))
                    break;
            }
            if (!operate(READ, 0))
                break;
            found.push_back(target[idx]);
            idx++;
            if (idx >= target.size())
                break;
            distance = (target[idx] - 1) - head;
            if (distance < 0)
                distance += 8000;
        }
    }
    op_end();
    vector<Request>& requests = RequestManager::getInstance()->getRequests();
    vector<Object>& objects = ObjectManager::getInstance()->getObjects();
    for (int i : found)
    {
        int complete_id = 0;
        for (int j = 1; j <= 3; j++)
        {
            if (disk_id == objects[blocks[i]].replica[j])
            {
                for (int k = 1; k <= objects[blocks[i]].size; k++)
                {
                    if (objects[blocks[i]].unit[j][k] == i)
                    {
                        complete_id = k;
                        break;
                    }
                }
            }
            continue;
        }
        requests[objects[blocks[i]].last_request_point].complete[complete_id] = 1;
    }
}

bool Disk::hasSpace(int size) {
    // 检查磁盘是否有足够的空间
    return true; // 伪代码实现
}

void Disk::store(int id, int tag, int size) {
    // 存储对象的逻辑
    // 伪代码实现
}