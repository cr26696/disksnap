#ifndef PERSUADE_THREAD_HPP
#define PERSUADE_THREAD_HPP
#include <set>
#include <unordered_set>
#include <unordered_map>
#include <vector>
#include "Disk.hpp"

class PersuadeThread
{
private:
	Disk *disk;

public:
	int job_count = 0;
	// 存储任务块
	std::vector<Request *> task_requests; // size 对应job_count;
	std::set<int> task_blocks;
	// 只会在删除对象时才会有内容 一帧结尾时调用end函数清空
	std::vector<Request *> canceled_requests;
	// 只会在执行查找后有内容
	std::vector<Request *> complete_requests;
	std::unordered_map<int, std::vector<Request *>> map_obj_requests; // obj_id -> (size, complete_parts)

public:
	PersuadeThread(Disk *disk_ptr); // 构造函数传入disk指针
	void add_req(int req_id, Object &info);
	void rmv_req(Object &info);
	void excute_find();

	int read_custom(int current_costom, int len);

	void end();
};

#endif