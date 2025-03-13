// Object.hpp
#ifndef OBJECT_HPP
#define OBJECT_HPP

#include <vector>
#include "MetaDefine.hpp"

struct Object
{
    int replica[REP_NUM]; // 第几个副本存放于哪个盘 注意这里索引 起止 1 2 3
    int *unit[4];   // 数组，存放对象的某个副本具体存放的子单元在硬盘中的地址
    int size;
    int last_request_point; // 最后一次请求任务
    bool is_delete;
};

#endif // OBJECT_HPP