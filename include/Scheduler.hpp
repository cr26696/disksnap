// Scheduler.hpp
#ifndef SCHEDULER_HPP
#define SCHEDULER_HPP

#include <unordered_map>
#include <vector>
#include <string>
#include <iostream>
#include "Request.hpp"
#include "Object.hpp"

class Scheduler
{
private:
    unordered_map<int, std::vector<int>> active_requests; // 对象编号，请求当前对象的所有请求id

public:
    bool add_request(int req_id);
    bool del_request(int req_id);
    std::vector<int> get_task_for_disk(int disk_id);
    void req_upload();
};

#endif // SCHEDULER_HPP