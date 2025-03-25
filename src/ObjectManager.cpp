#include "ObjectManager.hpp"
using namespace std;

ObjectManager* ObjectManager::instance = nullptr;
ObjectManager* ObjectManager::getInstance() {
    if (instance == nullptr) {
        instance = new ObjectManager(); // 懒加载创建实例
    }
    return instance;
}
ObjectManager::~ObjectManager() {
    delete instance;
    instance = nullptr;
}

// 获取对象集合的引用
vector<Object>& ObjectManager::getObjects() {
    return objects;
}

// 添加对象
void ObjectManager::addObject(const Object& obj) {
    std::lock_guard<std::mutex> lock(mutex_);
    objects.push_back(obj);
}