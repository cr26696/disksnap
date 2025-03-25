// Request.hpp
#ifndef REQUEST_HPP
#define REQUEST_HPP

#include <vector>

class Request
{
public:
    int id;
    int object_id;
    int size;
    std::vector<int> tofind_addrs; // 需要找的块的地址
    bool complete[6];
    int prev_id;

public:
    Request(int id, int object_id, int size, std::vector<int> tofind_addrs);
    bool is_complete();
};

#endif // REQUEST_HPP