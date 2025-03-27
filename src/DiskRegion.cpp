#include "DiskRegion.hpp"

using namespace std;
DiskRegion::DiskRegion(int start, int end) : start(start), end(end), free_blocks_size(end - start + 1)
{
	int size = end - start + 1;
	int temp_key = free_blocks_size > 5 ? 6 : free_blocks_size;
	auto node = make_shared<ListNode>(start, end);
	// 初始化第一个区域
	SectionList.push_back(node);
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

int DiskRegion::get_write_mode_flag(int size)
{
	if(SectionSet[size].size() > 0)
	{
		return COMPLET_WRITE;
	}
	else if(SectionSet[MAX_FREE_BLOCK_KEY].size() > 0)
	{
		return MAXFREE_WRITE;
	}
	else
	{
		return DISCRET_WRITE;
	}
}