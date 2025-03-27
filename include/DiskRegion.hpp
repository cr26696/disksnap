#ifndef DISKREGION_HPP
#define DISKREGION_HPP

#define MAX_FREE_BLOCK_KEY 6
#define COMPLET_WRITE 1
#define MAXFREE_WRITE 2
#define DISCRET_WRITE 3

#include <vector>
#include <map>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <list>
#include <memory>
#include "Object.hpp"
#include "Disk.hpp"

struct ListNode {
    int start;
    int end;
    ListNode(int s, int e) : start(s), end(e) {}

    // // 为了在 unordered_set 中使用，定义相等比较操作符
    // bool operator==(const ListNode& other) const {
    //     return start == other.start && end == other.end;
    // }
	bool operator<(const ListNode& other) const {
        return start < other.start;
    }
};
// 定义哈希函数
struct ListNodeHash {
    std::size_t operator()(const std::shared_ptr<ListNode>& node) const {
        return std::hash<int>()(node->start) ^ std::hash<int>()(node->end);
    }
};
class DiskRegion
{
	friend class Disk;

private:
	int region_start;
	int region_end;
	int free_blocks_size; // 持续维护，记录所有空闲块
	std::list<std::shared_ptr<ListNode>> SectionList;//按地址存储空闲区段
    std::unordered_set<std::shared_ptr<ListNode>, ListNodeHash> SectionSet[MAX_FREE_BLOCK_KEY + 1];//大于5的区段都存在6中，其他按大小存储
public:
	DiskRegion(int start, int end);
	std::vector<int> use_space(Replica *rep);
	void free_space(Replica *rep);
	int get_write_mode_flag(int size);
    void check_section_list();
    void check_section_list_error();
    void check_size_leagal(int flag);
};
#endif