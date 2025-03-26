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
    
    // 新增：预处理阶段读取的连续存储的频率数据
    int PeriodNum;  // 时间片数量
    vector<std::vector<int>> fre_del;   // 删除操作统计数据
    vector<std::vector<int>> fre_write; // 写入操作统计数据
    vector<std::vector<int>> fre_read;  // 读取操作统计数据
    vector<int> label_index;  // 全局变量，记录每个标签在磁盘中的起始位置
    vector<double> tag_ratio;

private:
    System(const System &) = delete;
    System &operator=(const System &) = delete;
    System(int TimeStampNum, int TagNum, int DiskNum, int DiskVolume, int TokenG);
    void init();
    void update_time();
    void timestamp_action();
    void delete_action();
    void read_action();
    void write_action();
    void label_oriented_storge(); // 标签存储
    void phase_end();
public:
    // 获取单例实例的静态方法
    static System &getInstance(int TimeStampNum, int TagNum, int DiskNum, int DiskVolume, int TokenG);
    // static System &getInstance();
    void run();
};
#endif // SYSTEM_HPP