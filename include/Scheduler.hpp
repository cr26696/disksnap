// Scheduler.hpp
#ifndef SCHEDULER_HPP
#define SCHEDULER_HPP

#include <algorithm>
#include <vector>
#include <unordered_map>
#include <mutex>

#include "PersuadeThread.hpp"

class Scheduler
{
private:
    static Scheduler *instance;

    std::unordered_map<int, std::vector<int>> active_requests; // 对象编号，请求当前对象的所有活跃请求id
    Scheduler();
    std::mutex mutex_;
    std::vector<PersuadeThread> job_threads;

public:
    static Scheduler &getInstance();
    PersuadeThread &get_disk_thread(int thread_index);
    bool add_request(int req_id, int obj_id);
    bool del_request(int req_id);
    std::unordered_map<int, std::vector<int>> &get_active_requests();
    std::vector<int> get_canceled_reqs_id();
    std::vector<int> get_complete_reqs_id();
    void excute_find();
    void req_upload();
    void end();
};

#endif // SCHEDULER_HPP