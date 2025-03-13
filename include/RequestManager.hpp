#ifndef REQUEST_MANAGER_HPP
#define REQUEST_MANAGER_HPP

#include <vector>
#include <mutex>
#include "Request.hpp"

/*
全局单例vector<Request> 方法都线程安全
*/

class RequestManager {
public:
    static RequestManager* getInstance() {
        if (instance == nullptr) {
            instance = new RequestManager(); // 懒加载创建实例
        }
        return instance;
    }

    ~RequestManager() {
        delete instance;
        instance = nullptr;
    }

    std::vector<Request>& getRequests() {
        return requests;
    }

    void addRequest(const Request& req) {
        std::lock_guard<std::mutex> lock(mutex_);
        requests.push_back(req);
    }

    Request& getRequest(int index) {
        std::lock_guard<std::mutex> lock(mutex_);
        return requests[index];
    }

    int getSize() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return requests.size();
    }

private:
    RequestManager() {}
    RequestManager(const RequestManager&) = delete;
    RequestManager& operator=(const RequestManager&) = delete;

    static RequestManager* instance;
    std::vector<Request> requests;
    mutable std::mutex mutex_;// 线程安全锁
};

RequestManager* RequestManager::instance = nullptr;
#endif