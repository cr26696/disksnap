#ifndef DISKREGION_HPP
#define DISKREGION_HPP

#define MAX_FREE_BLOCK_KEY 6
#include <vector>
#include <map>
#include <set>
#include <unordered_map>
#include "Object.hpp"
#include "Disk.hpp"

// struct Block
// {
// 	int obj_id = -1; // 对象id
// 	int part = -1;	 // 对象副本的第几块
// 	bool start = false;
// 	bool end = false;
// 	bool used = false;
// };
class DiskRegion
{
	friend class Disk;

private:
	int start;
	int end;
	int free_blocks_size; // 持续维护，记录所有空闲块
	
	struct PairCompare {
		template <typename T1, typename T2>
		bool operator()(const std::pair<T1, T2>& lhs, const std::pair<T1, T2>& rhs) const {
			return lhs.first < rhs.first;  // 仅比较 first 成员
		}
	};
	std::unordered_map<int, std::set<std::pair<int, int>, PairCompare>> free_blocks; // 空间长度 查询 区域起始结束地址
public:
	// std::vector<Block> blocks;
	DiskRegion(int start, int end);
	// int getFreeBlocks();
	std::vector<int> use_space(Replica *rep);
	void free_space(Replica *rep);
};
#endif