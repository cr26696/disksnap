#ifndef PERSUADE_THREAD_HPP
#define PERSUADE_THREAD_HPP
#include <set>
#include <unordered_set>
#include <unordered_map>
#include <vector>
#include "Request.hpp"
#include "Disk.hpp"
#include <fstream>
#include <string>



class PersuadeThread
{
private:
	Disk *disk;

public:
	//TODO 重心动态维护
	int job_center;//重心 
	int job_count = 0;
	// 存储任务块
	// std::vector<Request *> task_requests; // size 对应job_count;
	std::set<int> task_blocks;
	// 只会在删除对象时才会有内容 一帧结尾时调用end函数清空
	std::vector<Request *> canceled_requests;
	// 只会在执行查找后有内容
	std::vector<Request *> complete_requests;
	std::unordered_map<int, std::vector<Request *>> map_obj_requests;

public:
	PersuadeThread(Disk *disk_ptr); // 构造函数传入disk指针
	void add_req(Request *req);
	void rmv_req(Object &info);
	void excute_find();
	void updata_job_center(bool is_add, int addr);
	int read_custom(int current_costom, int len);

	void end();
};

#endif