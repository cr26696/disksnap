// Object.hpp
#ifndef OBJECT_HPP
#define OBJECT_HPP

#include <vector>
#include "MetaDefine.hpp"

struct Object
{
    int id, size, tag;
    int diskid_replica[REP_NUM]; // 第0 1 2个副本存储在的磁盘id
    Object(int id, int size, int tag) : id(id), size(size), tag(tag) {};
    Object(){};
};

#endif // OBJECT_HPP