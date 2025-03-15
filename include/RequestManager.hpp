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
    static RequestManager* getInstance();

    // ~RequestManager() {
    //     delete instance;
    //     instance = nullptr;
    // }

    std::vector<Request>& getRequests();

    void addRequest(const Request& req);

    Request& getRequest(int index);

    int getSize() const;

private:
    RequestManager() {}
    RequestManager(const RequestManager&) = delete;
    RequestManager& operator=(const RequestManager&) = delete;

    static RequestManager* instance;
    std::vector<Request> requests;
    mutable std::mutex mutex_;// 线程安全锁
};

#endif