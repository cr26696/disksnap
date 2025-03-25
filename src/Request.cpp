// Request.cpp
#include "Request.hpp"

Request::Request(int id, int object_id, int size,std::vector<int> tofind_addrs)
	: id(id),
	  object_id(object_id),
	  size(size),
	  tofind_addrs(tofind_addrs)
{
	// tofind_addrs.resize(size, 0); // 这里int 默认值会是0
	for (int i = 0; i < size; i++)
	{
		complete[i] = false;
	};
}
bool Request::is_complete()
{
	for (int i = 0; i < size; i++)
		if (!complete[i])
			return false;
	return true;
};