// Request.cpp
#include "Request.hpp"

Request::Request(int id,int object_id,int size):id(id),object_id(object_id),size(size){
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