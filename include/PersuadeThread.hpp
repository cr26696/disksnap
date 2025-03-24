#ifndef PERSUADE_THREAD_HPP
#define PERSUADE_THREAD_HPP
#include <set>
#include <unordered_set>
#include "Disk.hpp"
// 添加 执行 查找任务 记录与磁盘相关的任务信息
// 创建于diskmanager
// 存放对应磁盘的任务块，任务完成情况，实际的查找调用
class PersuadeThread
{
private:
	Disk *disk;

public:
	// 存储任务块
	std::unordered_set<Request *> task_requests; // size 对应job_count;
	std::set<int> task_blocks;
	// 只会在删除对象时才会有内容 一帧结尾时调用end函数清空
	std::vector<Request *> canceled_requests;
	// 只会在执行查找后有内容
	std::vector<Request *> complete_requests;

public:
	PersuadeThread(Disk *disk_ptr); // 构造函数传入disk指针
	void add_req(int req_id, Object &info);
	void rmv_req(Object &info);
	void excute_find();

	void end();
};

#endif