// MetaDefine.hpp
#ifndef METADEFINE_HPP
#define METADEFINE_HPP

#define MAX_DISK_NUM 10
#define MAX_DISK_SIZE 16384
#define MAX_OBJECT_NUM 100000 // 最大对象数量
#define MAX_REQUEST_NUM 30000000 //最大请求数量
#define REP_NUM 3 // 副本数量
#define FRE_PER_SLICING 1800 // 时间戳分段长度
#define EXTRA_TIME 105 // 额外时间

#define DEBUG_ENABLED 0
#define DEBUG_PRINT(var)                                                                                      \
    if (DEBUG_ENABLED)                                                                                        \
    {                                                                                                         \
        std::cout << "[D] " << #var << " = " << (var) << " (in " << __PRETTY_FUNCTION__ << ")" << std::endl; \
    }
#define DEBUG_LOG(msg)                       \
    if (DEBUG_ENABLED)                       \
    {                                        \
        std::cout << "[D]: " << msg << endl; \
    }

#endif // METADEFINE_HPP