// Request.cpp
#include "Request.hpp"

Request::Request(int id,int object_id):id(id),object_id(object_id){
	needs.resize(size);
	for(int i = 0; i < size; i++){
		needs[i] = 0;
	}
}
//在disk的add req中调用
void Request::init_status(int size)
{
	this->size = size;
	for(int i = 0; i < size; i++){
		complete[i] = false;
	}
}
bool Request::is_complete()
{
	for(int i = 0; i < size; i++)
		if(!complete[i])
			return false;
	return true;
};