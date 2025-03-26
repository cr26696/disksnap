#ifndef DISKREGION_HPP
#define DISKREGION_HPP

#include <vector>
#include <map>
#include "Object.hpp"
#include "Disk.hpp"
class DiskRegion
{
private:
	int start;
	int end;
	int free_blocks;								// 持续维护，记录所有空闲块
	std::multimap<int, std::pair<int, int>> region; // 空间长度 查询 区域起始结束地址
public:
	DiskRegion(int start, int end);
	int getFreeBlocks();
	void use_space(Replica &rep);
	void free_space(Replica &rep);
};
#endif