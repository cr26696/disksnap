#ifndef DISKREGION_HPP
#define DISKREGION_HPP

#include <vector>
#include <map>
#include "Object.hpp"
#include "Disk.hpp"
class DiskRegion{
	private:
		int start;
		int end;
		int free_block;//持续维护，记录所有空闲块
		std::multimap<int,std::pair<int,int>> region;//空间长度 查询 区域起始结束地址
	public:
	DiskRegion(int start,int end);
	store_obj(Replica &rep);
};
#endif