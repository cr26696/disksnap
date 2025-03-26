#ifndef DISKREGION_HPP
#define DISKREGION_HPP

#include <vector>
#include <map>
#include "Object.hpp"
#include "Disk.hpp"

struct Block
{
	int obj_id = -1; // 对象id
	int part = -1;	 // 对象副本的第几块
	bool start = false;
	bool end = false;
	bool used = false;
};
class DiskRegion
{
	friend class Disk;

private:
	int start;
	int end;
	int free_blocks; // 持续维护，记录所有空闲块
	std::vector<Block> blocks;
	std::multimap<int, std::pair<int, int>> free_region; // 空间长度 查询 区域起始结束地址
public:
	DiskRegion(int start, int end);
	// int getFreeBlocks();
	void use_space(Replica *rep);
	void free_space(Replica *rep);
};
#endif