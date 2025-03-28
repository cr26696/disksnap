#include "DiskRegion.hpp"

using namespace std;
DiskRegion::DiskRegion(int start, int end) : region_start(start), region_end(end), free_blocks_size(end - start + 1)
{
	int size = end - start + 1;
	int temp_key = free_blocks_size > 5 ? 6 : free_blocks_size;
	auto node = make_shared<ListNode>(start, end);
	// // 初始化第一个区域
	// SectionList.push_back(node);
	// SectionSet[temp_key].insert(node);
	// check_size_leagal(0);
	free_blocks.push_back(make_pair(start, end));
}

void DiskRegion::check_section_list()
{
	if (SectionList.empty())
	{
		std::cout << "SectionList is empty." << std::endl;
		return;
	}

	auto prev_it = SectionList.begin();
	auto current_it = std::next(prev_it);

	while (current_it != SectionList.end())
	{
		if ((*current_it)->start == (*prev_it)->start)
		{
			std::cout << "Error: Node at position " << std::distance(SectionList.begin(), current_it)
					  << " has start value " << (*current_it)->start
					  << " which is not less than the previous node's start value " << (*prev_it)->start
					  << std::endl;
		}
		prev_it = current_it;
		++current_it;
	}
}
void DiskRegion::check_section_list_error(int i)
{
	int total_length = 0;
	set<int> check_set;
	set<int> check_set_end;
	for (const auto &node : SectionList)
	{
		total_length += node->end - node->start + 1;
	}

	for (int jj = 1; jj <= 6; jj++)
	{
		for (auto it = SectionSet[jj].begin(); it != SectionSet[jj].end(); it++)
		{
			if (check_set.find((*it)->start) == check_set.end())
			{
				check_set.insert((*it)->start);
			}
			else
			{
				std::cout << "Error: Duplicate start value " << (*it)->start << " found in SectionSet." << std::endl;
			}
			if (check_set_end.find((*it)->end) == check_set_end.end())
			{
				check_set_end.insert((*it)->end);
			}
			else
			{
				std::cout << "Error: Duplicate start value " << (*it)->end << " found in SectionSet." << std::endl;
			}
		}
	}
	if (total_length != free_blocks_size)
	{
		std::cout << "Error: Total length of sections (" << total_length
				  << ") does not match free_blocks_size (" << free_blocks_size << ")." << std::endl;
	}
}
void DiskRegion::check_size_leagal(int flag)
{
	for (int i = 1; i <= 5; i++)
	{
		if (SectionSet[i].size() == 0)
			continue;
		for (auto it = SectionSet[i].begin(); it != SectionSet[i].end(); it++)
		{
			if ((*it)->end - (*it)->start + 1 != i)
			{
				std::cout << "Error: size of section is not legal" << std::endl;
			}
		}
	}
	if (SectionSet[6].size() == 0)
		return;
	for (auto it = SectionSet[6].begin(); it != SectionSet[6].end(); it++)
	{
		if ((*it)->end - (*it)->start + 1 <= 5)
		{
			std::cout << "Error: size of section is not legal" << std::endl;
		}
	}
}
/// @brief 分配副本大小的空闲区域，调整muiltimap
/// @param rep
vector<int> DiskRegion::use_space(Replica *rep)
{
	int obj_id = rep->info.id;
	int size = rep->info.size;
	vector<int> result_addr;
	// int current_write_point = 0;
	int temp_write_point = 0;
	typedef list<pair<int, int>>::iterator p_it; // 记录暂存时所选择空闲块的迭代器
	vector<pair<p_it, int>> temp_operate;		 // 记录暂存空闲块与块大小
	pair<p_it, int> not_discret_write;
	int MODE_FLAG = DISCRET_MODE;
	/* 进行写入模式判断 完全写入、富余写入、分段写入 */
	for (auto it = free_blocks.begin(); it != free_blocks.end(); it++)
	{
		int free_block_size = it->second - it->first + 1; // 当前空闲块的空间
		if (free_block_size == size)
		{
			MODE_FLAG = COMPLET_MODE;
			not_discret_write = make_pair(it, size);
			break;
		}
		else if (free_block_size > size && free_block_size >= 6)
		{
			MODE_FLAG = MAXFREE_MODE;
			not_discret_write = make_pair(it, size);
		}
		else
		{
			/*--------进行分块暂存记录 以防无法连续存储  ---------*/
			// TODO 改为优先使用碎片中的大号
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
	}
	/* 开始按照模式进行存储 */
	switch (MODE_FLAG)
	{
	case COMPLET_MODE:
	{
		p_it complet_block = not_discret_write.first; // it为所操作空闲块的迭代器
		int com_free_block_size = complet_block->second - complet_block->first + 1;
		assert(size == com_free_block_size);
		for (int i = 0; i < size; i++)
		{
			// blocks[complet_block->first + i] = std::make_pair(obj_id, i);
			// replica->addr_part[i] = complet_block->first + i;
			result_addr.push_back(complet_block->first + i);
		}
		free_blocks.erase(complet_block); // 完全写入模式中进行节点删除
		break;
	}
	case MAXFREE_MODE:
	{
		p_it maxfree_block = not_discret_write.first;
		int max_free_block_size = maxfree_block->second - maxfree_block->first + 1;
		assert(size < max_free_block_size);
		for (int i = 0; i < size; i++)
		{
			// blocks[maxfree_block->first + i] = std::make_pair(obj_id, i);
			// replica->addr_part[i] = maxfree_block->first + i;
			result_addr.push_back(maxfree_block->first + i);
		}
		maxfree_block->first += size; // 富余写入模式中进行节点修改
		break;
	}
	case DISCRET_MODE:
	{
		for (auto it : temp_operate)
		{
			int current_write_point = 0;
			int operate_write_size = it.second;
			p_it temp_operate_block = it.first;
			for (int i = 0; i < operate_write_size; i++)
			{
				// blocks[temp_operate_block->first + i] = std::make_pair(obj_id, current_write_point);
				// replica->addr_part[current_write_point] = temp_operate_block->first + i;
				result_addr.push_back(temp_operate_block->first + i);
				// current_write_point++;
			}
			int free_block_size = temp_operate_block->second - temp_operate_block->first + 1;
			if (operate_write_size == free_block_size)
			{ // 如果填满了整个空闲块，则删除该空闲块
				free_blocks.erase(temp_operate_block);
			}
			else
			{ // 否则修改空闲块的起始位置
				temp_operate_block->first += operate_write_size;
			}
		}
		break;
	}
	default:
	{
		throw std::runtime_error("Error:  Write Mode Error");
		break;
	}
	}
	free_blocks_size -= rep->info.size;
	return result_addr;
}

void DiskRegion::free_space(Replica *rep)
{
    int object_id = rep->info.id;
    int size = rep->info.size;
    // unordered_set<int> canceled_reqs = DiskManager::getInstance().canceled_reqs;
    // if (replicas[object_id] == nullptr)
    // {
    //     throw std::invalid_argument("del_obj failed,No object id in this disk");
    // }
    // Replica *replica = replicas[object_id];  // 找到对应的副本
    vector<int> temp_free_units;             // 存放空闲碎片
    vector<pair<int, int>> temp_free_blocks; // 存放合并空闲碎片后的空闲区间
    /*----------开始释放磁盘----------*/
    for (int i = 0; i < size; i++)
    {
        int addr = rep->addr_part[i];
        // assert(blocks[i] != nullptr);
        // blocks[addr].reset();
        temp_free_units.emplace_back(addr);
    }
    // replicas[object_id] = nullptr;
    // delete (replica);
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
	free_blocks_size += rep->info.size;
}

int DiskRegion::get_write_mode_flag(int size)
{
	if (SectionSet[size].size() > 0)
	{
		return COMPLET_MODE;
	}
	else if (SectionSet[MAX_FREE_BLOCK_KEY].size() > 0)
	{
		return MAXFREE_MODE;
	}
	else
	{
		return DISCRET_MODE;
	}
}

void DiskRegion::check_all(int i)
{
	check_section_list();
	check_section_list_error(i);
	check_size_leagal(i);
}