#ifndef UNIT_HPP
#define UNIT_HPP
#include "Replica.hpp"
class Replica;
class Unit
{
	// 具体放在磁盘里的对象片
	// 应该指向副本
private:
	Replica *replica;
	int part;
public:
	Unit(Replica *replica,int part);
};
#endif