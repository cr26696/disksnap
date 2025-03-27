#include "DiskRegion.hpp"

using namespace std;
DiskRegion::DiskRegion(int start, int end) : start(start), end(end), free_blocks_size(end - start + 1)
{
	int size = end - start + 1;
	int temp_key = free_blocks_size > 5 ? 6 : free_blocks_size;
	free_blocks[temp_key].insert(make_pair(start, end)); // 初始化第一个区域
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
				for(int i = 0; i < rep_size; i++)
				{
					result.push_back(area->first + i);// 返回修改的地址
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
				for(int i = 0; i < rep_size; i++)
				{
					result.push_back(area->first + i);// 返回修改的地址
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
			for(int j = 0; j < temp_size; j++)
			{
				result.push_back(temp_area.first + j);// 返回暂存方法里的地址
			}
			free_blocks[temp_size].erase(temp_area);
			if (free_blocks[temp_size].empty())
			{
				free_blocks.erase(temp_size); // 该种类中没有空闲块则删除该种类
			}
		}
		/* 最后一块单独处理 */
		pair<int, int> temp_area = temp_areas.back();
		for(int j = 0; j < reamain_size; j++)
		{
			result.push_back(temp_area.first + j);// 返回最后一块的地址
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
	for (auto it_map = free_blocks.begin(); it_map != free_blocks.end(); it_map++)
	{
		for (auto it_set = it_map->second.begin(); it_set != it_map->second.end();)
		{
			int area_start = it_set->first;	   // 区域起始地址
			int old_area_end = it_set->second; // 区域结束地址
			int new_area_end = old_area_end;   // 合并后的区域结束地址
			while (next(it_set) != it_map->second.end() && next(it_set)->first == it_set->second + 1)
			{
				new_area_end = next(it_set)->second;   // 找到合并块的尾巴
				it_set = it_map->second.erase(it_set); // 删除原区域
			}
			if (new_area_end != old_area_end) // 说明进行了合并
			{	
				it_set = it_map->second.erase(it_set); // 删除while退出时的最后一个连续区块
				int new_key = new_area_end - area_start + 1;
				if (new_key > MAX_FREE_BLOCK_KEY)
					new_key = MAX_FREE_BLOCK_KEY;
				free_blocks[new_key].insert(make_pair(area_start, new_area_end)); // 插入新区域
			}
			else // 没有进行合并 迭代器后移
			{
				it_set++;
			}
		}
	}
	free_blocks_size += rep->info.size;
	/* TODO是否需要删除空闲块为空的种类？*/
	for (auto it_map = free_blocks.begin(); it_map != free_blocks.end();)
	{
		if (it_map->second.empty())
		{
			it_map = free_blocks.erase(it_map);
		}
		else
		{
			it_map++;
		}
	}
}
