#include "DiskRegion.hpp"

using namespace std;
DiskRegion::DiskRegion(int start, int end) : region_start(start), region_end(end), free_blocks_size(end - start + 1)
{
	int size = end - start + 1;
	int temp_key = free_blocks_size > 5 ? 6 : free_blocks_size;
	auto node = make_shared<ListNode>(start, end);
	// 初始化第一个区域
	SectionList.push_back(node);
	SectionSet[temp_key].insert(node);
	check_size_leagal(0);
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
void DiskRegion::check_section_list_error()
{
	int total_length = 0;
	for (const auto &node : SectionList)
	{
		total_length += node->end - node->start + 1;
	}

	if (total_length != free_blocks_size)
	{
		std::cout << "Error: Total length of sections (" << total_length
				  << ") does not match free_blocks_size (" << free_blocks_size << ")." << std::endl;
	}
}
void DiskRegion::check_size_leagal(int flag){
	for(int i=1;i<=5;i++){
		if(SectionSet[i].size() == 0) continue;
		for(auto it = SectionSet[i].begin(); it != SectionSet[i].end(); it++){
			if((*it)->end - (*it)->start + 1 != i){
				std::cout << "Error: size of section is not legal" << std::endl;
			}
		}
	}
	if(SectionSet[6].size() == 0) return;
	for(auto it = SectionSet[6].begin(); it != SectionSet[6].end(); it++){
		if((*it)->end - (*it)->start + 1 <= 5){
			std::cout << "Error: size of section is not legal" << std::endl;
		}
	}
}
/// @brief 分配副本大小的空闲区域，调整muiltimap
/// @param rep
vector<int> DiskRegion::use_space(Replica *rep)
{
	check_section_list();
	check_section_list_error();
	check_size_leagal(1);
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
		SectionList.remove(*write_area_it);
		SectionSet[rep_size].erase(write_area_it);
		free_blocks_size -= rep->info.size; // 维护空闲块的总数量
		check_section_list();
		check_section_list_error();
		check_size_leagal(2);
		bool t = 1;
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
		(*write_area_it)->start += rep_size; // 修改区域起始地址
		check_size_leagal(3);
		int new_key = (*write_area_it)->end - (*write_area_it)->start + 1;
		if (new_key < MAX_FREE_BLOCK_KEY) // 若新的空闲块小于MAX_FREE_BLOCK_KEY，则插入到对应的set中
		{
			shared_ptr<ListNode> new_node = *write_area_it;
			auto it = lower_bound(SectionList.begin(), SectionList.end(), new_node); // 找到插入位置
			SectionList.insert(it, new_node);
			SectionList.remove(*write_area_it);
			SectionSet[new_key].insert(new_node);
			check_size_leagal(4);
			SectionSet[MAX_FREE_BLOCK_KEY].erase(write_area_it);
			check_size_leagal(5);
		}
		free_blocks_size -= rep->info.size; // 维护空闲块的总数量
		check_section_list();
		check_section_list_error();
		check_size_leagal(6);
		bool t = 1;
	}
	else if (wirte_mode_flag == DISCRET_WRITE)
	{
		int remain_size = rep_size;
		vector<pair<shared_ptr<ListNode>, int>> temp_opt; // shared_ptr<ListNode>:存储写入区域 int写入个数
		assert(SectionSet[MAX_FREE_BLOCK_KEY].size() == 0 && SectionSet[rep_size].size() == 0);
		// 考虑没有1的空闲块
		if (rep_size == 1)
		{
			int temp_key = 2;
			while (SectionSet[temp_key].size() == 0)
			{
				temp_key++;
			}
			auto write_area_it = SectionSet[temp_key].begin();
			result.emplace_back((*write_area_it)->start);
			(*write_area_it)->start++;
			shared_ptr<ListNode> new_node = *write_area_it;
			auto it = lower_bound(SectionList.begin(), SectionList.end(), new_node); // 找到插入位置
			SectionList.insert(it, new_node);
			SectionList.remove(*write_area_it);
			SectionSet[new_node->end - new_node->start + 1].insert(new_node);
			check_size_leagal(7);
			SectionSet[temp_key].erase(write_area_it);
			check_size_leagal(8);
			free_blocks_size -= rep_size;
			check_section_list();
			check_section_list_error();
			check_size_leagal(9);
			bool t = 1;
			return result;
		}
		assert(rep_size != 1);
		for (int discret_key = 5; remain_size > 0; discret_key--) // 进行离散存储选择
		{
			assert(discret_key >= 1);
			if (SectionSet[discret_key].size() == 0)
				continue;
			for (auto write_area_it = SectionSet[discret_key].begin(); write_area_it != SectionSet[discret_key].end(); write_area_it++)
			{
				if (remain_size - discret_key > 0)
					temp_opt.emplace_back(*write_area_it, discret_key);
				else if (remain_size - discret_key <= 0)
					temp_opt.emplace_back(*write_area_it, remain_size);
				remain_size -= discret_key;
				if (remain_size <= 0)
					break;
			}
		}
		for (int i = 0; i < temp_opt.size() - 1; i++) // 执行离散存储
		{
			for (int j = temp_opt[i].first->start; j <= temp_opt[i].first->end; j++)
			{
				result.emplace_back(j);
			}
			auto temp_write_area_it = SectionSet[temp_opt[i].second].find(temp_opt[i].first);
			assert(temp_write_area_it != SectionSet[temp_opt[i].second].end());
			SectionList.remove(*temp_write_area_it);
			SectionSet[temp_opt[i].second].erase(temp_write_area_it);
			check_size_leagal(10);
		}
		for (int j = temp_opt.back().first->start; j <= temp_opt.back().first->end; j++) // 最后的地址单独处理
		{
			result.emplace_back(j);
		}
		int last_opt_key = temp_opt.back().first->end - temp_opt.back().first->start + 1; // 最后的使用的空闲块移动位置
		auto temp_write_area_it = SectionSet[last_opt_key].find(temp_opt.back().first);
		check_section_list();
		check_section_list_error();
		check_size_leagal(11);
		assert(temp_write_area_it != SectionSet[last_opt_key].end());
		shared_ptr<ListNode> new_node = *temp_write_area_it;
		auto it = lower_bound(SectionList.begin(), SectionList.end(), new_node); // 找到插入位置
		SectionList.insert(it, new_node);
		SectionList.remove(*temp_write_area_it);
		assert(last_opt_key!=temp_opt.back().second);
		SectionSet[last_opt_key - temp_opt.back().second].insert(new_node);
		check_size_leagal(12);
		SectionSet[last_opt_key].erase(temp_write_area_it);
		check_size_leagal(13);
		free_blocks_size -= rep->info.size; // 维护空闲块的总数量
		check_section_list();
		check_section_list_error();
		check_size_leagal(14);
		bool t = 1;
	}
	else
	{
		throw invalid_argument("Invalid write mode flag");
	}
	
	check_section_list();
	check_section_list_error();
	check_size_leagal(15);
	return result;
}

void DiskRegion::free_space(Replica *rep)
{
	// 新增释放的空间
	// 尝试合并相邻块
	// 根据合成成功与否 删除或添加块
	// 将链表的操作同步到set中
	// 更新空闲块数量的记录
	check_section_list();
	check_section_list_error();
	check_size_leagal(16);
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
			if ((*it)->end + 1 == new_node->start) {
				// 处理与前区间邻接
				new_node->start = (*it)->start;
				temp_deletes.push_back(*it);
			}
			++it; // 回到插入位置
		}
		if (it != SectionList.end() && (*it)->start == new_node->end+1){
			// 存在后一个区间
			new_node->end = (*it)->end;								  // 处理与后区间邻接
			temp_deletes.push_back(*it);
		} 
		/* 删除被合并的区间 */
		for (int i = 0; i < temp_deletes.size(); i++)
		{
			SectionList.remove(temp_deletes[i]);
			int size_key = min(temp_deletes[i]->end - temp_deletes[i]->start + 1, 6);
			SectionSet[size_key].erase(temp_deletes[i]);
			check_size_leagal(17);
		}
		/* 插入新区间 */
		it = lower_bound(SectionList.begin(), SectionList.end(), new_node); // 找到插入位置
		SectionList.insert(it, new_node);
		int size_key = min(section.second - section.first + 1, 6);
		SectionSet[size_key].insert(new_node);
		check_size_leagal(18);
	}
	free_blocks_size += rep->info.size;
	if (SectionList.size() >= 2)
	{
		auto it = SectionList.begin();
		int i1 = (*it)->start;
		int i2 = (*next(it))->start;
		if (i1 == i2)
			throw invalid_argument("Invalid free space");
	}
	check_section_list();
	check_section_list_error();
	check_size_leagal(19);
}

int DiskRegion::get_write_mode_flag(int size)
{
	if (SectionSet[size].size() > 0)
	{
		return COMPLET_WRITE;
	}
	else if (SectionSet[MAX_FREE_BLOCK_KEY].size() > 0)
	{
		return MAXFREE_WRITE;
	}
	else
	{
		return DISCRET_WRITE;
	}
}