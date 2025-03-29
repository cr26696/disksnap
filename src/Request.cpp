// Request.cpp
#include "Request.hpp"

Request::Request(int id,int object_id,int size):id(id),object_id(object_id),size(size){
	for(int i = 0; i < size; i++){
		auto& unit = req_units[i];
		unit.pDisk = nullptr;
		unit.addr = -1;
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
void Request::suspend_request() {
	//TODO 更新文档得分函数 计算更新value
	//需要除块数量
	//低分请求直接放弃掉 删掉伪线程里的task blocks

	//先尝试 90帧直接舍弃
};