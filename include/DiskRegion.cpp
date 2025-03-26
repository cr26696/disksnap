#include "DiskRegion.hpp"

using namespace std;
DiskRegion::DiskRegion(int start, int end):start(start),end(end)
{
	int size = end - start + 1;
	region.insert({0,make_pair(start, end)});//初始化第一个区域
}