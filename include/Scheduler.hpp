// Scheduler.hpp
#ifndef SCHEDULER_HPP
#define SCHEDULER_HPP

#include <algorithm>
#include <vector>
#include <unordered_map>
#include <mutex>
#include "PersuadeThread.hpp"
extern int t;
class Scheduler
{
    friend class Request;
    friend class System;

private:
    static Scheduler *instance;
    int last_check_time = 1;
    std::unordered_map<int, std::vector<int>> active_requests; // 对象编号，请求当前对象的所有活跃请求id
    Scheduler();
    std::mutex mutex_;
    std::vector<PersuadeThread> job_threads;
    void check_suspend(const int current_time);

public:
    static Scheduler &getInstance();
    PersuadeThread &get_disk_thread(int thread_index);
    bool add_request(int req_id, int obj_id);
    bool del_request(int req_id);
    std::unordered_map<int, std::vector<int>> &get_active_requests();
    std::vector<int> get_canceled_reqs_id();
    std::vector<int> get_complete_reqs_id();
    void excute_find();
    std::string getUploadInfo();
    double job_rating(std::vector<int> addrs, int job_center);
    double rateReqToDisk(int disk_head, std::vector<int> &addrs, int job_center, int job_count, int volume);
    void end();
};

#endif // SCHEDULER_HPP