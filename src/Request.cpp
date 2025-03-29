// Request.cpp
#include "Request.hpp"
#include "Scheduler.hpp"
Request::Request(int id,int object_id,int size, int time):id(id),object_id(object_id),size(size),add_time(time){
	req_complete_time = -1;
	for(int i = 0; i < size; i++){
		auto& unit = req_units[i];
		unit.pDisk = nullptr;
		unit.addr = -1;
		unit.find_time = -1;
		unit.complete = false;
	}
}

bool Request::is_complete()
{
	//TODO 改判断完成
	for(int i = 0; i < size; i++)
		if(req_units[i].complete == false)
			return false;
	return true;
}
int Request::suspend_request() {
	//TODO 更新文档得分函数 计算更新value
	//需要除块数量
	//低分请求直接放弃掉 删掉伪线程里的task blocks
	//先尝试 90帧直接舍弃
	const int current_time = t;
	Scheduler &SD = Scheduler::getInstance();
	assert(current_time >= add_time);
	int temp_disk_id = req_units[0].pDisk->id;
	if(current_time - add_time >= MAX_ALIVE_TIME)
	{
		for(int i = 0; i < size; i++)// 进行task_blocks剔除
		{
			int temp_addr = req_units[i].addr;
			// TODO取消前判断是否为同一个对象
			// if(SD.disk[temp_disk_id].blocks[temp_addr].obj_id == req_units[i].pDisk->blocks[temp_addr].obj_id){
				SD.job_threads[temp_disk_id].task_blocks.erase(temp_addr);
			// }
		}
		return size;
	}
	return 0;
};

