#ifndef REQUEST_HPP
#define REQUEST_HPP
#include <vector>
#include "Disk.hpp"
#define MAX_ALIVE_TIME 90
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
    int dnot_comp_num;   // 仍未完成的数量
    bool legacy; // 是否为已抛弃请求
    int req_complete_time;
public:
    Request(int id, int object_id, int size, int time);
    bool is_complete();
    double get_sorce(int current_time);
    int suspend_request();
    int do_not_complete_num();
};

#endif // REQUEST_HPP