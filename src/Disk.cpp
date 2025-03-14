// Disk.cpp
#include "Disk.hpp"
using namespace std;
// 构造函数
Disk::Disk(int V, int G)
    : head(0), head_s(-1), sizeV(V), tokenG(G), elapsed(0), phase_end(false), blocks(V, 0)
{
    free_blocks.emplace_back(1, V);
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


/*
* 删除对象 并进行空闲块合并整理 按区间开头升序排序
*/
void Disk::delete_obj(int *units, int object_size){
    vector<int> temp_free_units;// 空闲碎片
    vector<pair<int, int>> temp_free_blocks;// 空闲区间
    for(int i = 1; i <= object_size; i++){
        assert(blocks[units[i]] != 0);
        blocks[units[i]] = 0;
        temp_free_units.emplace_back(units[i]);
    }
    sort(temp_free_units.begin(), temp_free_units.end());// 将碎片块排序
    int start = temp_free_units[0], end = temp_free_units[0];
    for(int i = 1; i < temp_free_units.size(); i++){// 将空闲碎片合并为空闲区间
        if(temp_free_units[i] == end + 1){
            end = temp_free_units[i];
        }
        else if(temp_free_units[i] != end + 1 || i == temp_free_units.size() - 1){// 若当前块与前一个块不连续或为最后一个区间
            temp_free_blocks.emplace_back(start, end);
            start = temp_free_units[i];
            end = temp_free_units[i];
        }
    }
    for(auto& block : temp_free_blocks) {
        free_blocks.push_back(block);
    }
    free_blocks.sort();
    for(auto current_block = free_blocks.begin(); current_block != free_blocks.end();){
        auto next_block = next(current_block);
        if(next_block == free_blocks.end()) break;
        assert(current_block->second == next_block->first - 1);// 理论上不会大于
        if(current_block->second == next_block->first - 1){// 表示上一个空闲区别和下一个空闲区间相近 需要进行合并
            current_block->second = next_block->second;
            free_blocks.erase(next_block);
        }
        else{
            current_block++;
        }
    }    
}


/*
* 写入对象 优先满足连续存储 若无法连续存储 则进行分块存储
*/
void Disk::write_obj(int object_id, int *obj_units, int object_size){
    int current_write_point = 0;
    int temp_write_point = 0;
    typedef list<pair<int, int>>::iterator p_it;// 记录暂存时所选择空闲块的迭代器
    vector<pair<p_it, int>> temp_operate;//记录暂存空闲块与块大小
    for(auto it = free_blocks.begin(); it != free_blocks.end(); it++){
        int free_block_size = it->second - it->first + 1;// 当前空闲块的空间
        // 找到可连续存储块时 放弃暂存 直接存储
        if(free_block_size >= object_size){
            for(int i = 0; i < object_size; i++){// 从0开始 因为可从首地址开始存储
                assert(blocks[it->first + i] == 0);
                blocks[it->first + i] = object_id;// 存入磁盘
                obj_units[++current_write_point] = it->first + i;// 标记对象块位置
            }
            if(free_block_size == object_size){// 若空闲块被填满，则删除该空闲块节点
                free_blocks.erase(it);
            }
            else{// 若空闲块没有被填满，则修改该空闲块区间头
                it->first += object_size;
            }
            assert(current_write_point == object_size);
            return;
        }
        // 进行分块暂存记录 以防无法连续存储
        if(temp_write_point != object_size){
            int not_write_size = object_size - temp_write_point;// 仍未存入的对象块大小
            // 标记暂存空间块使用情况 填满空闲块：填入剩余对象块剩余空间
            if(not_write_size >= free_block_size){
                temp_operate.emplace_back(it, free_block_size);
                temp_write_point += free_block_size;
            }
            else{
                temp_operate.emplace_back(it, not_write_size);
                temp_write_point += not_write_size;
            }
        }        
    }
    // 使用暂存操作
    for(auto it : temp_operate){
        for(int i = 0; i < it.second; i++){
            assert(blocks[it.first->first + i] == 0);
            blocks[it.first->first + i] = object_id;// 存入磁盘
            obj_units[++current_write_point] = it.first->first + i;// 标记对象块位置
        }
        int free_block_size = it.first->second - it.first->first + 1;
        if (it.second == free_block_size) { // 如果填满了整个空闲块，则删除该空闲块
            free_blocks.erase(it.first);
        } else { // 否则修改空闲块的起始位置
            it.first->first += it.second;
        }        
    }
    assert(current_write_point == object_size);
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

/*
*计算总剩余空间大小
*/
int Disk::numberOfFreeBlocks_() {
    int all_free_size = 0;
    for(auto it : free_blocks){
        all_free_size += it.second - it.first + 1;
    }
    return all_free_size;
}

