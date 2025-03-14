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
    std::vector<int> needs;
    bool complete[6];
    int prev_id;

    public:
    Request(int id,int object_id);
    void init_status(int size);
    bool is_complete();
};

#endif // REQUEST_HPP