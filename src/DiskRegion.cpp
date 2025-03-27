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
/// @param rep
vector<int> DiskRegion::use_space(Replica *rep)
{
	int wirte_mode_flag;
	vector<int> result;
	int rep_size = rep->info.size;
	assert(rep_size <= free_blocks_size);

	/* 进行写入策略判断 */
	wirte_mode_flag = get_write_mode_flag(rep_size);

	/* 按照模式进行写入 */
	if (wirte_mode_flag == COMPLET_WRITE)
	{
		assert(SectionSet[rep_size].size() > 0);
		// auto &write_area_set = SectionSet[rep_size];
		auto write_area_it = SectionSet[rep_size].begin();
		for (int i = 0; i < rep_size; i++)
		{
			result.emplace_back((*write_area_it)->start + i);
		}
		SectionSet[rep_size].erase(write_area_it);
	}
	else if (wirte_mode_flag == MAXFREE_WRITE)
	{
		assert(SectionSet[MAX_FREE_BLOCK_KEY].size() > 0);
		// auto &write_area_set = SectionSet[MAX_FREE_BLOCK_KEY];
		auto write_area_it = SectionSet[MAX_FREE_BLOCK_KEY].begin();
		for (int i = 0; i < rep_size; i++)
		{
			result.emplace_back((*write_area_it)->start + i);
		}
		(*write_area_it)->start += rep_size;// 修改区域起始地址
		int new_key = (*write_area_it)->end - (*write_area_it)->start + 1;
		if(new_key < MAX_FREE_BLOCK_KEY)// 若新的空闲块小于MAX_FREE_BLOCK_KEY，则插入到对应的set中
		{
			shared_ptr<ListNode> new_node = *write_area_it;
			SectionSet[new_key].insert(new_node);
			SectionSet[MAX_FREE_BLOCK_KEY].erase(write_area_it);
		}
	}
	else if (wirte_mode_flag == DISCRET_WRITE)
	{
		int remain_size = rep_size;
		vector<pair<shared_ptr<ListNode>, int>> temp_opt;// shared_ptr<ListNode>:存储写入区域 int写入个数
		assert(SectionSet[MAX_FREE_BLOCK_KEY].size() == 0 && SectionSet[rep_size].size() == 0);
		for(int discret_key = rep_size - 1; remain_size > 0; discret_key--)// 进行离散存储选择
		{
			assert(discret_key >= 1);
			if(SectionSet[discret_key].size() == 0) continue;
			for(auto write_area_it = SectionSet[discret_key].begin(); write_area_it != SectionSet[discret_key].end(); write_area_it++)
			{
				if(remain_size - discret_key > 0) temp_opt.emplace_back(*write_area_it, discret_key);
				else if(remain_size - discret_key <= 0) temp_opt.emplace_back(*write_area_it, remain_size);
				remain_size -= discret_key;
				if(remain_size <= 0) break;
			}
		}
		for(int i = 0; i < temp_opt.size() - 1; i++)// 执行离散存储
		{
			for(int j = temp_opt[i].first->start; j <= temp_opt[i].first->start; j++)
			{
				result.emplace_back(j);
			}
			auto temp_write_area_it = SectionSet[temp_opt[i].second].find(temp_opt[i].first);
			assert(temp_write_area_it != SectionSet[temp_opt[i].second].end());
			SectionSet[temp_opt[i].second].erase(temp_write_area_it);
		}
		for(int j = temp_opt.back().first->start; j <= temp_opt.back().first->start; j++)// 最后的地址单独处理
		{
			result.emplace_back(j);
		}
		int last_opt_key = temp_opt.back().first->end - temp_opt.back().first->start + 1;// 最后的使用的空闲块移动位置
		auto temp_write_area_it = SectionSet[last_opt_key].find(temp_opt.back().first);
		assert(temp_write_area_it != SectionSet[last_opt_key].find(temp_opt.back().first));
		shared_ptr<ListNode> new_node = *temp_write_area_it;
		SectionSet[last_opt_key - temp_opt.back().second].insert(new_node);
		SectionSet[last_opt_key].erase(temp_write_area_it);
	}
	else
	{
		throw invalid_argument("Invalid write mode flag");
	}
	free_blocks_size -= rep->info.size;// 维护空闲块的总数量
	return result;
}

void DiskRegion::free_space(Replica *rep)
{
	return;
}

int DiskRegion::get_write_mode_flag(int size)
{
	if (SectionSet[size].empty())
	{
		return COMPLET_WRITE; // 完全写入
	}
	else if (SectionSet[size].empty())
	{
		return MAXFREE_WRITE; // 富余写入
	}
	else
	{
		return DISCRET_WRITE; // 离散写入及其它情况
	}
}
