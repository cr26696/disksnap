// ObjectManager.hpp
#ifndef OBJECTMANAGER_HPP
#define OBJECTMANAGER_HPP

#include <vector>
#include "Object.hpp"
#include <mutex>

class ObjectManager {
private:
    static ObjectManager* instance; // 静态指针成员
    std::vector<Object> objects; // 管理的Object对象集合
    mutable std::mutex mutex_;
    ObjectManager() {} // 私有构造函数，防止外部实例化

public:
    static ObjectManager* getInstance();

    ~ObjectManager();

    // 获取对象集合的引用
    std::vector<Object>& getObjects();

    // 添加对象
    void addObject(const Object& obj);

};


#endif // OBJECTMANAGER_HPP