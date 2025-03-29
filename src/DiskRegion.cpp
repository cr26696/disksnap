#include "DiskRegion.hpp"
#define DEBUG_MODE true
using namespace std;
DiskRegion::DiskRegion(int start, int end) : region_start(start), region_end(end), free_blocks_size(end - start + 1)
{
	int size = end - start + 1;
	int temp_key = free_blocks_size > 5 ? 6 : free_blocks_size;
	auto node = make_shared<ListNode>(start, end);
	// 初始化第一个区域
	SectionList.push_back(node);
	SectionSet[temp_key].insert(node);
	illegal_check("DiskRegion constructor check size legal");
}
void DiskRegion::illegal_check(string text)
{
	check_list_order(text);
	check_list_length(text);
	check_set_key(text);
}
void DiskRegion::check_list_order(string text)
{
	if (SectionList.empty())
	{
		return;
	}

	auto prev_it = SectionList.begin();
	auto current_it = std::next(prev_it);

	while (current_it != SectionList.end())
	{
		if ((*current_it)->start <= (*prev_it)->start)
		{
			cout << text << endl;
			throw std::range_error("Error:Ordering\nNode" + std::to_string(std::distance(SectionList.begin(), current_it)) +
								   " start at" + std::to_string((*current_it)->start) +
								   " but prev start at" + std::to_string((*prev_it)->start));
		}
		prev_it = current_it;
		++current_it;
	}
}
void DiskRegion::check_list_length(string text)
{
	int total_length = 0;
	for (const auto &node : SectionList)
	{
		total_length += node->end - node->start + 1;
	}

	if (total_length != free_blocks_size)
	{
		cout << text << endl;
		throw std::range_error("Error:Length\ntotal length of section is " + to_string(total_length) +
							   " but free_blocks_size is " + to_string(free_blocks_size));
	}
}
void DiskRegion::check_set_key(string text)
{
	for (int i = 1; i <= 5; i++)
	{
		if (SectionSet[i].size() == 0)
			continue;
		for (auto it = SectionSet[i].begin(); it != SectionSet[i].end(); it++)
		{
			int start = (*it)->start;
			int end = (*it)->end;
			int len = end - start + 1;
			if (len != i)
			{
				cout << text << endl;
				throw std::range_error("Error:setkey\nsection (" + to_string(start) + "~" + to_string(end) + ")" +
									   "length is" + to_string(len) + "but in SectionSet[" + to_string(i) + "]");
			}
		}
	}
	if (SectionSet[6].size() == 0)
		return;
	for (auto it = SectionSet[6].begin(); it != SectionSet[6].end(); it++)
	{
		if ((*it)->end - (*it)->start + 1 <= 5)
		{
			cout << text << endl;
			throw std::range_error("size of sectionset not matching its key");
		}
	}
}
void DiskRegion::check_set_size(std::string text)
{
	int sum =0;
	for(int i=1;i<=6;i++){
		sum+=SectionSet[i].size() * i;
	}
	if(sum!=free_blocks_size)
	throw std::range_error("size of sectionset not matching its key");
}
// 同时新往list与set中插入新section
void DiskRegion::insert_section(shared_ptr<ListNode> section)
{
	int len = section->end - section->start + 1;
	int insert_key = min(len, 6);
	SectionSet[insert_key].insert(section);
	auto it_pos = lower_bound(SectionList.begin(), SectionList.end(), section, [](const shared_ptr<ListNode> &a, const shared_ptr<ListNode> &b)
							  { return a->start < b->start; });
	SectionList.insert(it_pos, section);
	free_blocks_size += len;
}
// 从set和list中删除section
// 注意Section由智能指针控制自动释放
void DiskRegion::delete_section(shared_ptr<ListNode> section)
{
	if (DEBUG_MODE)
	{
		auto it_list = find_iter_of_section(section);
		assert(it_list != SectionList.end());
		int key = min(section->end - section->start + 1, 6);
		assert(SectionSet[key].find(section) != SectionSet[key].end());
	}
	SectionList.remove(section);
	int len = section->end - section->start + 1;
	int set_key = min(len, 6);
	int set_erase_num = SectionSet[set_key].erase(section);
	assert(set_erase_num != 0);
	free_blocks_size -= len;
}
// 需要section已在list与set中，进行调整（对于set会移动位置）,并改变free_blocks_size
void DiskRegion::resize_section(shared_ptr<ListNode> section, int new_start, int new_end)
{
	if (section->start == new_start && section->end == new_end) {
		return;
	}
	if(DEBUG_MODE){
		auto it_list = find_iter_of_section(section);
		assert(it_list != SectionList.end());
		int key = min(section->end - section->start + 1, 6);
		assert(SectionSet[key].find(section) != SectionSet[key].end());
	}
	int old_len = section->end - section->start + 1;
	int new_len = new_end - new_start + 1;
	int old_key = min(old_len, 6);
	int new_key = min(new_len, 6);
	int set_erase_num = this->SectionSet[old_key].erase(section);
	assert(set_erase_num != 0);
	section->start = new_start;
	section->end = new_end;
	SectionSet[new_key].insert(section);
	free_blocks_size += new_len - old_len;
}
// 输入section，与请求使用的大小，返回存放的地址向量,并重设section大小
vector<int> DiskRegion::use_section(shared_ptr<ListNode> section, int size)
{
	//OPT 如标签区域已满，向相邻标签区域存储
	if(DEBUG_MODE){
		auto it_list = find_iter_of_section(section);
		assert(it_list != SectionList.end());
		int key = min(section->end - section->start + 1, 6);
		assert(SectionSet[key].find(section) != SectionSet[key].end());
	}
	vector<int> addrs;
	addrs.reserve(size);
	int start = section->start;
	int end = section->end;
	int len = end - start + 1;
	if (size > len)
		throw std::range_error("section size smaller than request");
	for (int i = 0; i < size; i++)
	{
		addrs.push_back(start + i);
	}
	if (len == size)
	{
		delete_section(section);
	}
	else
	{
		resize_section(section, start + size, end);
	}
	return addrs;
}
// 查找section在list中的位置 找不到会报错
list<std::shared_ptr<ListNode>>::iterator DiskRegion::find_iter_of_section(shared_ptr<ListNode> section)
{
	auto it_sec = SectionList.begin();
	while (*it_sec != section)
	{
		it_sec++;
		if (it_sec == SectionList.end())
			throw std::range_error("section not found");
	}
	return it_sec;
}
// 这里要求section已在list中
void DiskRegion::merge_neighbors(shared_ptr<ListNode> section)
{
	auto it_sec = find_iter_of_section(section);
	list<std::shared_ptr<ListNode>>::iterator it_lnode;
	list<std::shared_ptr<ListNode>>::iterator it_rnode;
	int new_start = (*it_sec)->start;
	int new_end = (*it_sec)->end;
	if (it_sec != SectionList.begin())
	{
		it_lnode = prev(it_sec);
		if ((*it_lnode)->end + 1 == (*it_sec)->start)
		{
			new_start = (*it_lnode)->start;
			delete_section(*it_lnode);
		}
	}
	it_rnode = next(it_sec);
	if (it_rnode != SectionList.end())
	{
		if ((*it_sec)->end + 1 == (*it_rnode)->start)
		{
			new_end = (*it_rnode)->end;
			delete_section(*it_rnode);
		}
	}
	resize_section(*it_sec, new_start, new_end);
}

vector<int> DiskRegion::use_space(int size)
{
	if (free_blocks_size < size)
		throw invalid_argument("has no enough size");
	illegal_check("use_space start");
	vector<int> result;
	if (size > 5 || size < 1)
	{
		throw std::range_error("size of obj impossible");
	}
	if (size > free_blocks_size)
	{
		throw std::logic_error("can't alloc enough space");
	}
	/* 恰好存下 */
	if (SectionSet[size].size() != 0)
	{
		shared_ptr<ListNode> section = *(SectionSet[size].begin());
		vector<int> addrs = use_section(section, size);
		illegal_check("use_space perfect");
		return addrs;
	}
	/* 使用最大空间存 */
	if (SectionSet[6].size() != 0)
	{
		shared_ptr<ListNode> section = *SectionSet[6].begin();
		vector<int> addrs = use_section(section, size);
		illegal_check("use_space max_section");
		return addrs;
	}
	/* 使用多个小于6的块存 */
	vector<shared_ptr<ListNode>> to_insert_sections;
	to_insert_sections.reserve(size);
	int remain = size;
	int curent_key = 5;
	auto it = SectionSet[curent_key].begin();
	while (remain > 0)
	{
		if (it == SectionSet[curent_key].end())
		{
			curent_key--;
			it = SectionSet[curent_key].begin();
			continue;
		}
		else
		{
			to_insert_sections.push_back(*it);
			assert((*it)->end - (*it)->start + 1 == curent_key);
			remain -= curent_key;
			++it;
		}
	}
	for (int i = 0; i < to_insert_sections.size()-1;i++) {
		auto node = to_insert_sections[i];
		int len = node->end - node->start +1;
		vector<int> addrs = use_section(node, len);
		result.insert(result.end(), addrs.begin(), addrs.end());
	}
	/* 最后一个只使用部分长度 */
	auto node = to_insert_sections.back();
	int len = node->end - node->start + 1;
vector<int> addrs = use_section(node, remain+len);
	result.insert(result.end(), addrs.begin(), addrs.end());
	sort(result.begin(), result.end());
	illegal_check("use_space end");
	return result;
}

void DiskRegion::free_space(vector<int> &addrs)
{
	// 新增释放的空间
	// 尝试合并相邻块
	// 根据合成成功与否 删除或添加块
	// 将链表的操作同步到set中
	// 更新空闲块数量的记录
	illegal_check("free_space start");
	int size = addrs.size();
	if (size == 0)
		throw invalid_argument("try to free no space");
	if (size == 1)
	{
		shared_ptr<ListNode> new_sec = make_shared<ListNode>(addrs[0], addrs[0]);
		insert_section(new_sec);
		merge_neighbors(new_sec);
		illegal_check("free_space after free 1");
		return;
	}
	int addr = addrs[0];
	int start = addr;
	int end = addr;
	vector<shared_ptr<ListNode>> to_insert_sections;
	to_insert_sections.reserve(size);
	/* 合并空闲空间 减少操作次数 */
	for (size_t i = 1; i < size; i++)
	{
		addr = addrs[i];
		if (addr == end + 1)
			end = addr;
		else
		{

			to_insert_sections.push_back(make_shared<ListNode>(start, end));
			start = addr;
			end = addr;
		}
	}
	to_insert_sections.push_back(make_shared<ListNode>(start, end)); // 结尾空间单独插入（整个为一个区间也在此插入）
	for (auto &section : to_insert_sections)
	{
		insert_section(section);
		merge_neighbors(section);
	}
	// free_blocks_size += size;
	illegal_check("free_space end");
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
