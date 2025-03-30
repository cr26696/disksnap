#ifndef REQUEST_HPP
#define REQUEST_HPP
#include <vector>
#include "Disk.hpp"

extern int t;

struct RequestUnit
{
    Disk *pDisk;
    int addr;
    bool complete;
    int find_time;
};
class Request
{
public:
    int id;
    int object_id;
    int size;
    RequestUnit req_units[6];
    int add_time;
    int value;   // 价值
    bool legacy; // 是否为已抛弃请求
    int req_complete_time;
public:
    Request(int id, int object_id, int size, int time);
    bool is_complete();
    int suspend_request();
};

#endif // REQUEST_HPP