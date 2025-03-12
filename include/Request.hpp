// Request.hpp
#ifndef REQUEST_HPP
#define REQUEST_HPP

struct Request
{
    int object_id;
    int prev_id;
    bool is_done;
    bool complete[6] = {0};
};

#endif // REQUEST_HPP