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
void DiskRegion::check_section_list_error(int i)
{
	int total_length = 0;
	set<int> check_set;
	set<int> check_set_end;
	for (const auto &node : SectionList)
	{
		total_length += node->end - node->start + 1;
	}

	for(int jj = 1; jj <= 6; jj++){
		for(auto it = SectionSet[jj].begin(); it != SectionSet[jj].end(); it++){
			if(check_set.find((*it)->start) == check_set.end()){
				check_set.insert((*it)->start);
			}
			else{
				std::cout << "Error: Duplicate start value " << (*it)->start << " found in SectionSet." << std::endl;
			}
			if(check_set_end.find((*it)->end) == check_set_end.end()){
				check_set_end.insert((*it)->end);
			}
			else{
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
	check_all(1);
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
		assert(write_area_it != SectionSet[rep_size].end());
		assert((*write_area_it)->end - (*write_area_it)->start + 1 == rep_size);
		for (int i = 0; i < rep_size; i++)
		{
			result.emplace_back((*write_area_it)->start + i);
		}
		// check_all(2);
		SectionList.remove(*write_area_it);
		// SectionSet[rep_size].erase(write_area_it);
		free_blocks_size -= rep->info.size; // 维护空闲块的总数量
		// check_all(3);
	}
	else if (wirte_mode_flag == MAXFREE_WRITE)
	{
		assert(SectionSet[MAX_FREE_BLOCK_KEY].size() > 0);
		// auto &write_area_set = SectionSet[MAX_FREE_BLOCK_KEY];
		auto write_area_it = SectionSet[MAX_FREE_BLOCK_KEY].begin();
		assert(write_area_it != SectionSet[MAX_FREE_BLOCK_KEY].end());
		assert((*write_area_it)->end - (*write_area_it)->start + 1 > rep_size);
		for (int i = 0; i < rep_size; i++)
		{
			result.emplace_back((*write_area_it)->start + i);
		}
		// check_all(4);
		// (*write_area_it)->start += rep_size; // 修改区域起始地址
		shared_ptr<ListNode> new_node = make_shared<ListNode>((*write_area_it)->start + rep_size, (*write_area_it)->end);
		SectionList.remove(*write_area_it);
		SectionList.push_back(new_node);
		// int new_key = (*write_area_it)->end - (*write_area_it)->start + 1;
		// if (new_key < MAX_FREE_BLOCK_KEY) // 若新的空闲块小于MAX_FREE_BLOCK_KEY，则插入到对应的set中
		// {
		// 	SectionSet[new_key].insert(*write_area_it);
		// 	SectionSet[MAX_FREE_BLOCK_KEY].erase(write_area_it);
		// }
		free_blocks_size -= rep->info.size; // 维护空闲块的总数量
		// check_all(5);
	}
	else if (wirte_mode_flag == DISCRET_WRITE)
	{
		int remain_size = rep_size;
		vector<pair<shared_ptr<ListNode>, int>> temp_opt; // shared_ptr<ListNode>:存储写入区域 int写入个数
		assert(SectionSet[MAX_FREE_BLOCK_KEY].size() == 0 && SectionSet[rep_size].size() == 0);
		for (int discret_key = 5; remain_size > 0; discret_key--) // 进行离散存储选择
		{
			assert(discret_key >= 1);
			if (SectionSet[discret_key].size() == 0)
				continue;
			for (auto write_area_it = SectionSet[discret_key].begin(); write_area_it != SectionSet[discret_key].end(); write_area_it++)
			{
				if (remain_size - discret_key > 0)
				{
					temp_opt.emplace_back(*write_area_it, discret_key);// 操作的空闲块与空闲块的使用量 用完
					remain_size -= discret_key;
				}
					
				else if (remain_size - discret_key <= 0){
					temp_opt.emplace_back(*write_area_it, remain_size);// 操作的空闲块与空闲块的使用量 可能未用完
					remain_size -= discret_key;
				}
				if (remain_size <= 0)
					break;
			}
		}
		// check_all(6);
		for (int i = 0; i < temp_opt.size() - 1; i++) // 执行离散存储
		{
			for (int j = temp_opt[i].first->start; j <= temp_opt[i].first->end; j++)
			{
				result.emplace_back(j);
			}
			auto temp_write_area_it = SectionSet[temp_opt[i].second].find(temp_opt[i].first);// 找到所操作的空闲块的迭代器
			assert(temp_write_area_it != SectionSet[temp_opt[i].second].end());
			SectionList.remove(*temp_write_area_it);// 进行删除
			// SectionSet[temp_opt[i].second].erase(temp_write_area_it);
		}
		/* 离散存储的最后一个操作地址单独处理 */
		for (int j = temp_opt.back().first->start; j <= temp_opt.back().first->end; j++) 
		{
			result.emplace_back(j);
		}
		int last_opt_key = temp_opt.back().first->end - temp_opt.back().first->start + 1; // 最后的使用的空闲块的种类即key
		auto temp_write_area_it = SectionSet[last_opt_key].find(temp_opt.back().first);
		assert(temp_write_area_it != SectionSet[last_opt_key].end());
		// assert(last_opt_key < MAX_FREE_BLOCK_KEY);
		int new_last_opt_key = min(6, last_opt_key - temp_opt.back().second);// 被使用后的空闲块的种类即key
		if(new_last_opt_key == 0)// 最后的操作空闲块中已经被使用完 只要删除不需要移动
		{
			SectionList.remove(*temp_write_area_it);
			// SectionSet[last_opt_key].erase(temp_write_area_it);
		}
		else// 最后的操作空闲块中还没有被使用完 需要移动
		{
			assert(last_opt_key!=temp_opt.back().second);// 保证最后一次操作的空闲块没有被用完 需要移动
			// (*temp_write_area_it)->start += temp_opt.back().second;
			shared_ptr<ListNode> new_node = make_shared<ListNode>((*temp_write_area_it)->start + temp_opt.back().second, (*temp_write_area_it)->end);
			SectionList.remove(*temp_write_area_it);
			SectionList.push_back(new_node);
			// shared_ptr<ListNode> new_node = *temp_write_area_it;
			// SectionSet[new_last_opt_key].insert(new_node);
			// SectionSet[last_opt_key].erase(temp_write_area_it);
		}
		free_blocks_size -= rep->info.size; // 维护空闲块的总数量
		// check_all(7);
	}
	else
	{
		throw invalid_argument("Invalid write mode flag");
	}
	
	SectionList.sort([](const std::shared_ptr<ListNode>& a, const std::shared_ptr<ListNode>& b) {
        return a->start < b->start;
    });
	/* 清空当前unordered_set 重新进行计算 */
	for (int i = 1; i <= MAX_FREE_BLOCK_KEY; ++i) {
		SectionSet[i].clear();  
	}
	for(auto& it : SectionList)
	{
		int temp_key = min(it->end - it->start + 1, 6);
		SectionSet[temp_key].insert(it);
	}
	// check_all(8);

	return result;
}

void DiskRegion::free_space(Replica *rep)
{
	// 新增释放的空间
	// 尝试合并相邻块
	// 根据合成成功与否 删除或添加块
	// 将链表的操作同步到set中
	// 更新空闲块数量的记录
	// check_all(9);
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
	for (auto &section : to_insert_sections)
	{
		shared_ptr<ListNode> new_node = make_shared<ListNode>(section.first, section.second);
		SectionList.push_back(new_node);
		SectionSet[min(section.second - section.first + 1, 6)].insert(new_node);
	}
	SectionList.sort([](const std::shared_ptr<ListNode>& a, const std::shared_ptr<ListNode>& b) {
        return a->start < b->start;
    });
	for(auto it = SectionList.begin(); it != SectionList.end();){
		auto next_it = next(it);
		if(next_it == SectionList.end()) break;
		if((*it)->end == (*next_it)->start - 1){
			// int temp_it_key = min((*it)->end - (*it)->start + 1, 6);
			// int temp_next_key = min((*next_it)->end - (*next_it)->start + 1, 6);
			int temp_end = (*next(it))->end;;
			// auto to_delete1 = SectionSet[temp_next_key].find(*next_it);
			// auto to_delete2 = SectionSet[temp_it_key].find(*it);
			// assert(to_delete1 != SectionSet[temp_next_key].end());
			// assert(to_delete2 != SectionSet[temp_next_key].end());
			// SectionSet[temp_next_key].erase(to_delete1);
			// SectionSet[temp_it_key].erase(to_delete2);
			(*it)->end = temp_end;
			int temp_new_key = min((*it)->end - (*it)->start + 1, 6);
			// SectionSet[temp_new_key].insert(*it);
			SectionList.erase(next_it);//
		}
		else{
			it++;
		}
	}
	/* 清空当前unordered_set 重新进行计算 */
	for (int i = 1; i <= MAX_FREE_BLOCK_KEY; ++i) {
		SectionSet[i].clear();  
	}
	for(auto& it : SectionList)
	{
		int temp_key = min(it->end - it->start + 1, 6);
		SectionSet[temp_key].insert(it);
	}
	free_blocks_size += rep->info.size;
	check_all(10);
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

void DiskRegion::check_all(int i){
	check_section_list();
	check_section_list_error(i);
	check_size_leagal(i);
}