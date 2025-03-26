#include "DiskRegion.hpp"

using namespace std;
DiskRegion::DiskRegion(int start, int end) : start(start), end(end)
{
	int size = end - start + 1;
	free_region.insert({0, make_pair(start, end)}); // 初始化第一个区域
	blocks.resize(size);
	blocks[0].start = true;
	blocks[size-1].end = true;
};
/// @brief 分配副本大小的空闲区域，调整muiltimap
/// @param rep
void DiskRegion::use_space(Replica *rep)
{	//OPT 不考虑存不下的情况
	// TODO 存储
	// 先尝试向大小刚好合适的区段存
	// 如果没有这样的区段，查找分割更大的区段存
	// 如果没有更大的区段，分段存储

	// 实际要做的：删除或分割使用的区域
}

void DiskRegion::free_space(Replica *rep)
{
	// TODO 删除

	// 实际要做的：根据rep占据的块，释放区域
}
