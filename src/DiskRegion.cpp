#include "DiskRegion.hpp"

using namespace std;
DiskRegion::DiskRegion(int start, int end) : start(start), end(end), free_blocks_size(end - start + 1)
{
	int size = end - start + 1;
	int temp_key = free_blocks_size > 5 ? 6 : free_blocks_size;
	auto node = make_shared<ListNode>(start, end);
	// 初始化第一个区域
	SectionList.push_back(node);
	SectionSet[temp_key].insert(node);
}

/// @brief 分配副本大小的空闲区域，调整muiltimap
// TODO 存储
/// @param rep
vector<int> DiskRegion::use_space(Replica *rep)
{
	vector<int> result;
	int rep_size = rep->info.size;
	assert(rep_size <= free_blocks_size); // 保证有足够的空间存储
	int whole_write_key = rep_size;
	bool whole_write_flag = false;
	/* 判断是否可以连续存储 */
	if (free_blocks.find(whole_write_key) != free_blocks.end())
	{ // 找到了完全塞满的空闲块
		whole_write_flag = true;
	}
	else if (free_blocks.find(MAX_FREE_BLOCK_KEY) != free_blocks.end())
	{ // 使用最大号的空闲块
		whole_write_key = MAX_FREE_BLOCK_KEY;
		whole_write_flag = true;
	}
	else
	{
		whole_write_flag = false; // 无法连续存储
	}
	/* 可进行连续存储 */
	if (whole_write_flag == true)
	{
		auto it = free_blocks.find(whole_write_key);			   // it->second指向vector<pair<int, int>>
		assert(it != free_blocks.end() && it->second.size() != 0); // 保证找到了空闲块

		/* 只迭代一次 对第一个空闲块进行存储 */
		for (auto area = it->second.begin(); area != it->second.end();)
		{
			/* 在可填满空闲块中填充 需要删除该空闲块 */
			if (whole_write_key != MAX_FREE_BLOCK_KEY)
			{
				for (int i = 0; i < rep_size; i++)
				{
					result.push_back(area->first + i); // 返回修改的地址
				}
				free_blocks[whole_write_key].erase(area); // 删除该空闲块
				if (free_blocks[whole_write_key].empty())
				{
					free_blocks.erase(whole_write_key); // 该种类中没有空闲块则删除该种类
				}
			}
			/* 在最大号的空闲块中填充 需要修改该空闲块 */
			else
			{
				for (int i = 0; i < rep_size; i++)
				{
					result.push_back(area->first + i); // 返回修改的地址
				}
				pair<int, int> new_area = make_pair(area->first + rep_size, area->second);
				int remain_area_size = new_area.second - new_area.first + 1;
				if (remain_area_size < MAX_FREE_BLOCK_KEY)
				{
					free_blocks[remain_area_size].insert(new_area); // 更新块的key值
					free_blocks[MAX_FREE_BLOCK_KEY].erase(area);	// 删除该空闲块
					if (free_blocks[MAX_FREE_BLOCK_KEY].empty())
					{
						free_blocks.erase(MAX_FREE_BLOCK_KEY); // 该种类中没有空闲块则删除该种类
					}
				}
				else
				{
					free_blocks[MAX_FREE_BLOCK_KEY].erase(area);	  // 删除该空闲块
					free_blocks[MAX_FREE_BLOCK_KEY].insert(new_area); // 更新块的key值
				}
			}
			break;
		}
		free_blocks_size -= rep_size;
	}
	/* 进行分块存储 */
	else
	{
		int segment_key = rep_size - 1; // 分块存储 从最小的块开始
		int reamain_size = rep_size;
		vector<pair<int, int>> temp_areas;
		for (; segment_key > 0; segment_key--)
		{
			auto it_map = free_blocks.find(segment_key);
			if (it_map == free_blocks.end())
				continue;
			for (auto area = it_map->second.begin(); area != it_map->second.end(); area++)
			{
				temp_areas.emplace_back(area->first, area->second);
				reamain_size -= segment_key;
				if (reamain_size <= 0)
					goto outer_loop_end;
			}
		}
	outer_loop_end:
		reamain_size = rep_size;
		for (int i = 0; i < temp_areas.size() - 1; i++)
		{
			int temp_size = temp_areas[i].second - temp_areas[i].first + 1;
			pair<int, int> temp_area = temp_areas[i];
			for (int j = 0; j < temp_size; j++)
			{
				result.push_back(temp_area.first + j); // 返回暂存方法里的地址
			}
			free_blocks[temp_size].erase(temp_area);
			if (free_blocks[temp_size].empty())
			{
				free_blocks.erase(temp_size); // 该种类中没有空闲块则删除该种类
			}
		}
		/* 最后一块单独处理 */
		pair<int, int> temp_area = temp_areas.back();
		for (int j = 0; j < reamain_size; j++)
		{
			result.push_back(temp_area.first + j); // 返回最后一块的地址
		}
		pair<int, int> new_area = make_pair(temp_area.first + reamain_size, temp_area.second);
		free_blocks[temp_area.second - temp_area.first + 1].erase(temp_area);
		if (free_blocks[temp_area.second - temp_area.first + 1].empty())
		{
			free_blocks.erase(temp_area.second - temp_area.first + 1); // 该种类中没有空闲块则删除该种类
		}
		free_blocks[new_area.second - new_area.first + 1].insert(new_area);
		free_blocks_size -= rep_size;
	}
	return result;
}

void DiskRegion::free_space(Replica *rep)
{
	// 新增释放的空间
	// 尝试合并相邻块
	// 根据合成成功与否 删除或添加块
	// 将链表的操作同步到set中
	// 更新空闲块数量的记录
	assert(rep != nullptr);
	int addr = rep->addr_part[0];
	int start = addr;
	int end = addr;
	vector<pair<int, int>> to_insert_sections;
	to_insert_sections.reserve(rep->info.size);
	/* 合并空闲空间 减少操作次数 */
	for (size_t i = 1; i < rep->info.size; i++)
	{
		addr = rep->addr_part[i];
		if (addr == end + 1)
			end = addr;
		else
		{
			int size_key = min(end - start + 1, 6);
			to_insert_sections.emplace_back(start, end);
			start = addr;
			end = addr;
		}
	}
	to_insert_sections.emplace_back(start, end); // 结尾空间单独插入（整个为一个区间也在此插入）
	/* 遍历插入区段 */
	for (auto &section : to_insert_sections)
	{
		auto new_node = make_shared<ListNode>(section.first, section.second);
		auto it = lower_bound(SectionList.begin(), SectionList.end(), new_node); // 找到插入位置
		vector<shared_ptr<ListNode>> temp_deletes;
		if (it != SectionList.begin()) // 存在前一个区间
		{
			--it;							   // 指向前一个区间
			if ((*it)->end == new_node->start) // 处理与前区间邻接
				new_node->start = (*it)->start;
			++it; // 回到插入位置
		}
		if (it != SectionList.end() && (*it)->start == new_node->end) // 存在后一个区间
			new_node->end = (*it)->end;								  // 处理与后区间邻接
		/* 删除被合并的区间 */
		for (int i = 0; i < temp_deletes.size(); i++)
		{
			SectionList.remove(temp_deletes[i]);
			int size_key = min(temp_deletes[i]->end - temp_deletes[i]->start + 1, 6);
			SectionSet[size_key].erase(temp_deletes[i]);
		}
		/* 插入新区间 */
		SectionList.insert(it, new_node);
		int size_key = min(section.second - section.first + 1, 6);
		SectionSet[size_key].insert(new_node);
	}
	free_blocks_size += rep->info.size;
}
