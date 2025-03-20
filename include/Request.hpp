// Request.hpp
#ifndef REQUEST_HPP
#define REQUEST_HPP

#include<vector>

class Request
{
    public:
    int id;
    int object_id;
    int size;
    //TODO 优化成位运算
    bool complete[6];
    std::vector<int> needs;
    int prev_id;

    public:
    Request(int id,int object_id,int size);
    bool is_complete();
};

#endif // REQUEST_HPP