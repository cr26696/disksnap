// Scheduler.hpp
#ifndef SCHEDULER_HPP
#define SCHEDULER_HPP

#include <unordered_map>
#include <vector>
#include <string>
#include <algorithm>
#include <iostream>
#include "Request.hpp"
#include "Object.hpp"
#include "RequestManager.hpp" 
#include "ObjectManager.hpp"
#include "PersuadeThread.hpp"
#include <mutex>

class Scheduler
{
private:
    std::unordered_map<int, std::vector<int>> active_requests; // 对象编号，请求当前对象的所有活跃请求id
    
    Scheduler() = default;
    ~Scheduler() = default;
    static Scheduler* instance;
    std::mutex mutex_;
public:
    static Scheduler& getInstance();
    bool add_request(int req_id,int obj_id);
    bool del_request(int req_id);
    std::vector<int> get_task_for_disk(int disk_id);
    std::unordered_map<int, std::vector<int>>& get_active_requests();
    void req_upload();
};

#endif //SCHEDULER_HPP