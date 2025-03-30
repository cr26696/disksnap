#ifndef UTILS_HPP
#define UTILS_HPP
#include "algorithm"
int minDistance(int addr1, int addr2, int volume)
{
	int direct_distance = abs(addr1 - addr2);
	int wrap_distance = volume - direct_distance;
	return std::min(direct_distance, wrap_distance);
}
#endif
