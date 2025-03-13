#include "RequestManager.hpp"
using namespace std;

RequestManager* RequestManager::instance = nullptr;
RequestManager* RequestManager:: getInstance() {
    if (instance == nullptr) {
        instance = new RequestManager(); // 懒加载创建实例
    }
    return instance;
}


vector<Request>& RequestManager:: getRequests() {
    return requests;
}

void RequestManager::addRequest(const Request& req) {
    std::lock_guard<std::mutex> lock(mutex_);
    requests.push_back(req);
}

Request& RequestManager::getRequest(int index) {
    std::lock_guard<std::mutex> lock(mutex_);
    return requests[index];
}

int RequestManager::getSize() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return requests.size();
}