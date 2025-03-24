// System.hpp
#ifndef SYSTEM_HPP
#define SYSTEM_HPP

#include <vector>
#include <cstdio>
#include <cassert>
#include <cstdlib>
#include <string>
#include <algorithm>
#include <unordered_set>
#include <iostream>
#include <math.h>
#include "MetaDefine.hpp"
#include "Replica.hpp"
#include "PersuadeThread.hpp"
#include "Scheduler.hpp"
#include "DiskManager.hpp"
class System
{
private:
    int TimeStampNum;//数据总帧数
    int TagNum;
    int DiskNum;
    int DiskVolume;
    int TokenG;//时间片内时间长度
    int TimeStamp=0;//当前时间戳

private:
    System(const System &) = delete;
    System &operator=(const System &) = delete;
    System(int TimeStampNum, int TagNum, int DiskNum, int DiskVolume, int TokenG);

    void update_time();
    void timestamp_action();
    void delete_action();
    void read_action();
    void write_action();
    void phase_end();
public:
    // 获取单例实例的静态方法
    static System &getInstance(int TimeStampNum, int TagNum, int DiskNum, int DiskVolume, int TokenG);
    // static System &getInstance();
    void run();
};
#endif // SYSTEM_HPP