#include <cstdio>
#include <cassert>
#include <cstdlib>

#include <vector>
#include <string>
#include <algorithm>
#include <unordered_set>
#include <iostream>
#include <math.h>

using namespace std;

#define MAX_DISK_NUM (10 + 1)
#define MAX_DISK_SIZE (16384 + 1)
#define MAX_REQUEST_NUM (30000000 + 1)
#define MAX_OBJECT_NUM (100000 + 1)
#define REP_NUM (3)
#define FRE_PER_SLICING (1800)
#define EXTRA_TIME (105)
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

typedef struct Request_
{
    int object_id;
    int prev_id;
    bool is_done;
    bool complete[6] = {0};
} Request;

typedef struct Object_
{
    int replica[REP_NUM + 1]; // 第几个副本存放于哪个盘 注意这里索引 起止 1 2 3
    int *unit[REP_NUM + 1];   // 数组，存放对象的某个副本具体存放的子单元在硬盘中的地址
    int size;
    int last_request_point; // 最后一次请求任务
    bool is_delete;
} Object;
// TODO 将对象和对象副本解耦
Request request[MAX_REQUEST_NUM];
Object object[MAX_OBJECT_NUM];
int T, M, N, V, G;                     // 时间片T 标签M 硬盘N 存储单元数V 磁头tokenG
int disk[MAX_DISK_NUM][MAX_DISK_SIZE]; // 磁盘内容
int disk_point[MAX_DISK_NUM];          // 磁头位置
// 收集返回结果  统计已完成任务，最终统一上报
class Scheduler
{
public:
    unordered_set<int> active_requests; // OPT 改成map（对象编号，请求当前对象的所有请求id）
private:
public:
    // 成功失败
    bool add_request(int req_id)
    {
        pair<unordered_set<int>::iterator, bool> result = active_requests.insert(req_id);
        return result.second;
    }
    bool del_request(int req_id)
    {
        return active_requests.erase(req_id);
    }
    // 返回的vector从0开始索引
    vector<int> get_task_for_disk(int disk_id)
    {
        vector<int> target;
        // target.push_back(-1);

        for (int req_id : active_requests)
        {

            Request req = request[req_id];
            Object obj = object[req.object_id];

            for (int i = 1; i <= 3; i++)
            {
                if (obj.replica[i] == disk_id)
                {
                    int *units = obj.unit[i];
                    for (int i = 1; i <= obj.size; i++)
                    {
                        target.push_back(units[i]);
                    }
                    break;
                }
            }
        }
        sort(target.begin(), target.end());
        DEBUG_PRINT(target.size());
        return target;
    }
    void req_upload()
    {
        int complete_num = 0;
        string info = "";
        bool complete_flag;

        for (auto actreq_id : active_requests)
        {
            complete_flag = true; // 初始化为true
            for (int i = 1; i <= object[request[actreq_id].object_id].size; i++)
            {
                if (request[actreq_id].complete[i] != true)
                    complete_flag = false; // 如果有不完整的，设置为false
            }
            if (complete_flag)
            {
                complete_num++;
                del_request(actreq_id);
                request[actreq_id].is_done = true;
                info += std::to_string(actreq_id) + "\n"; // 转换为字符串
            }
        }
        info = std::to_string(complete_num) + "\n" + info; // 转换complete_num为字符串
        cout << info;
        return;
    }
};
enum DiskOp
{
    PASS,
    JUMP,
    READ,
};
class Disk
{

    int head = 0;    // 初始为0 如size == 8000 可能的值为0~7999
    int head_s = -1; // 磁头上个操作 -1初始化 0刚jump过 >0
    // 64刚read过 52=ceil(64*0.8) ... 16 上次连续read过
    // 1 pass过
    int size;
    int blocks[MAX_DISK_SIZE]; // 考虑这里就存 obj_id吗，需不需要其他信息？
    int token;                 // 当前时间片总可用token数
    int elapsed = 0;           // 当前时间片已用token数
    int phase_end = false;
    // OPT 是否可以维护一个hash表 键为obj_id 值为一些关键信息 如各块存放位置

private:
    // 会移动head JUMP需传入跳转地址 PASS传入距离 READ无视参数可填0
    // 返回操作是否成功 pass 0也算成功
    bool operate(DiskOp op, int param)
    {
        // DEBUG_PRINT(op);
        // DEBUG_PRINT(param);
        int consume_token = 0;

        switch (op)
        {
        case JUMP:
            DEBUG_LOG("ready jump");
            if (elapsed > 0)
            {
                phase_end = true;
                return false;
            }
            printf("j %d\n", param+1);
            head = param % V;
            head_s = 0;
            elapsed = token+1;
            phase_end = true;
            break;
        case PASS:
            // 如果给的距离超过了当前时间能移动的上限，会尽量移动（这样算优化吗？）
            if (param >= 1)
            {
                if (elapsed + param > token)
                {
                    phase_end = true;
                    return false;
                }
                string s = string(param, 'p');
                printf("%s", s.c_str()); // 使用 c_str() 将 std::string 转换为 const char*
                head += param;
                head = head % size;
                head_s = 1;
                elapsed += param;
                return true;
            } // pass 0等于不做动作
            else if(param==0) return true;
            return false;
            break;
        case READ:
            // DEBUG_PRINT(head_s);
            // DEBUG_PRINT(elapsed);
            // DEBUG_PRINT(token);
            consume_token = 64;
            if (head_s > 16)
                consume_token = head_s;
            if (elapsed + consume_token >= token)
            {
                phase_end = true;
                return false;
            }
            else
            {
                elapsed += consume_token;
                head_s = ceil(consume_token*0.8);
                printf("r");
                head++;
            }
            break;
        default:
            break;
        }
        return true;
    }
    void op_end()
    {   
        if(elapsed==token+1){
            elapsed = 0;
            phase_end = false;
            return;
        }
        printf("#\n");
        elapsed = 0;
        phase_end = false;
    }

public:
    void init()
    {
        head = 0;
        token = G;
        size = V;
    }
    void delete_obj(int *units, int size)
    {
        for (int i = 1; i <= size; i++)
        {
            blocks[units[i]] = 0;
        }
    }
    void write_obj(int object_id, int *obj_units, int size)
    {
        int current_write_point = 0;
        for (int i = 1; i <= V; i++)
        {
            if (blocks[i] == 0)
            {
                blocks[i] = object_id;
                obj_units[++current_write_point] = i;
                if (current_write_point == size)
                {
                    break;
                }
            }
        }
        assert(current_write_point == size);
    }

    // 传入需要读取的 磁盘的块 的索引 vector从0开始索引
    // 返回 已读取的块 的索引
    void task(vector<int> input_target, int disk_id)
    {   
        DEBUG_PRINT(disk_id);
        DEBUG_PRINT(head);
        DEBUG_PRINT(elapsed);
        if (input_target.empty())
        {
            op_end();
            DEBUG_PRINT(input_target.size());
            return;
        }
        // DEBUG_LOG("each block target for this disk");
        // for (auto blockidx : input_target)
        // {
        //     DEBUG_PRINT(blockidx);
        // }
        // 如果无任务，直接结束
        vector<int> target=input_target;
        // 尝试对input target按离磁头远近重排
        DEBUG_PRINT(head);
        auto it = lower_bound(target.begin(), target.end(), head+1);
        if (it == target.begin() || it == target.end())
        {
            // 所有元素都在磁头之前、之后 都不用排
            DEBUG_LOG("neednt REarrage");
        }
        else
        {
            vector<int> first_half(target.begin(), it);
            vector<int> second_half(it, target.end());
            target.clear();
            target.insert(target.end(), second_half.begin(), second_half.end());
            target.insert(target.end(), first_half.begin(), first_half.end());
        }
        // DEBUG_LOG("each block target after rearrange");
        // for (auto blockidx : target)
        // {
        //     DEBUG_PRINT(blockidx);
        // }
        // OPT 调用最优化使用磁头token的算法
        // 第一次出现的目标大于(令牌-64)就进行jump 否则pass至目标 进行读取
        int idx = 0; // target[idx]可用得到下一个目标的磁盘地址
        int tokens = G;
        int need_move = false;
        vector<int> found;
        DEBUG_PRINT(target[idx]);
        // target[0]是磁头后 最近的目标块
        // 距离 target[0]-head dis = dist<0?dist+8000:dist
        // 如 target[0]=2 head = 7998 距离应为4 直接减为 -7996
        int distance = (target[0]-1) - head;
        if (distance < 0)
            distance += 8000;
        DEBUG_PRINT(distance);
        if (distance + 64 > tokens)
        {
            // 过远的情况
            operate(JUMP, target[0]-1);
            op_end();
            return;
        }
        else
        {
            while (elapsed < tokens) // elapsed+距离 < token上限
            {
                if(distance>0){
                    if (!operate(PASS, distance)) // token不足移动 直接退出循环
                    break;
                }
                //现在磁头移动到目标块上了
                int read_consume;
                if (!operate(READ, 0)) // token不足读取 退出循环
                    break;
                found.push_back(target[idx]); // 放入读到的块
                idx++;
                if (idx >= target.size()) // idx已经超过有元素的索引值了
                    break;
                // 更新distance 尝试找下一个
                distance = (target[idx]-1) - head;
                if (distance < 0)
                    distance += 8000;
            }
        }
        op_end();
        for (int i : found)
        { // i disk地址
            int complete_id = 0;
            for (int j = 1; j <= 3; j++)
            { // j 副本索引
                if (disk_id == object[blocks[i]].replica[j])
                {
                    for (int k = 1; k <= object[blocks[i]].size; k++)
                    { // k 第j副本的第k子块
                        if (object[blocks[i]].unit[j][k] == i)
                        {
                            complete_id = k;
                            break;
                        }
                    }
                }
                continue;
            }
            request[object[blocks[i]].last_request_point].complete[complete_id] = 1;
        }
        // return found;
    }
};

Scheduler scheduler;
Disk disks[MAX_DISK_NUM];
void timestamp_action()
{
    int timestamp;
    scanf("%*s%d", &timestamp);
    printf("TIMESTAMP %d\n", timestamp);

    fflush(stdout);
}

// void do_object_delete(const int* object_unit, int* disk_unit, int size)
// {
//     for (int i = 1; i <= size; i++) {
//         disk_unit[object_unit[i]] = 0;
//     }
// }

void delete_action()
{
    int n_delete;
    int abort_num = 0;
    static int _id[MAX_OBJECT_NUM];

    scanf("%d", &n_delete);
    for (int i = 1; i <= n_delete; i++)
    {
        scanf("%d", &_id[i]);
    }

    // 删除时 链式取消request
    for (int i = 1; i <= n_delete; i++)
    {
        int id = _id[i];
        int current_id = object[id].last_request_point;

        while (current_id != 0)
        {
            // TODE 调用调度器的um查找_id的对象编号 值清空)
            if (request[current_id].is_done == false)
            {
                scheduler.del_request(current_id);
                abort_num++;
            }
            current_id = request[current_id].prev_id;
        }
    }

    printf("%d\n", abort_num);
    // 具体abort 了哪些编号的request
    for (int i = 1; i <= n_delete; i++)
    {
        int id = _id[i];
        int current_id = object[id].last_request_point;
        while (current_id != 0)
        {
            if (request[current_id].is_done == false)
            {
                printf("%d\n", current_id);
            }
            current_id = request[current_id].prev_id;
        }
        // 删除所有副本
        for (int j = 1; j <= REP_NUM; j++)
        {
            disks[object[id].replica[j]].delete_obj(object[id].unit[j], object[id].size);
            // do_object_delete(object[id].unit[j], disk[object[id].replica[j]], object[id].size);
        }
        object[id].is_delete = true;
    }

    fflush(stdout);
}

// void do_object_write(int* object_unit, int* disk_unit, int size, int object_id)
// {
//     int current_write_point = 0;
//     for (int i = 1; i <= V; i++) {
//         if (disk_unit[i] == 0) {
//             disk_unit[i] = object_id;
//             object_unit[++current_write_point] = i;
//             if (current_write_point == size) {
//                 break;
//             }
//         }
//     }

//     assert(current_write_point == size);
// }

void write_action()
{
    int n_write;
    scanf("%d", &n_write);
    for (int i = 1; i <= n_write; i++)
    {
        int id, size;
        scanf("%d%d%*d", &id, &size);
        object[id].last_request_point = 0;
        for (int j = 1; j <= REP_NUM; j++)
        {
            object[id].replica[j] = (id + j) % N + 1;
            object[id].unit[j] = static_cast<int *>(malloc(sizeof(int) * (size + 1)));
            object[id].size = size;
            object[id].is_delete = false;
            disks[(id + j) % N + 1].write_obj(id, object[id].unit[j], size);
            // do_object_write(object[id].unit[j], disk[object[id].replica[j]], size, id);
        }

        printf("%d\n", id);
        for (int j = 1; j <= REP_NUM; j++)
        {
            printf("%d", object[id].replica[j]);
            for (int k = 1; k <= size; k++)
            {
                printf(" %d", object[id].unit[j][k]);
            }
            printf("\n");
        }
    }

    fflush(stdout);
}

void read_action()
{
    int n_read;
    int request_id, object_id;
    scanf("%d", &n_read);
    for (int i = 1; i <= n_read; i++)
    {
        scanf("%d%d", &request_id, &object_id);
        request[request_id].object_id = object_id;
        request[request_id].prev_id = object[object_id].last_request_point;
        object[object_id].last_request_point = request_id;
        request[request_id].is_done = false;
        scheduler.add_request(request_id);
    }
    // for(auto req_id:scheduler.active_requests){
    //     DEBUG_PRINT(req_id);
    // }
    for (int i = 1; i <= N; i++)
    {
        disks[i].task(scheduler.get_task_for_disk(i), i);
    }
    scheduler.req_upload();

    // static int current_request = 0;
    // static int current_phase = 0;
    // if (!current_request && n_read > 0)
    // {
    //     current_request = request_id;
    // }
    // if (!current_request)
    // {
    //     for (int i = 1; i <= N; i++)
    //     {
    //         printf("#\n");
    //     }
    //     printf("0\n");
    // }
    // else
    // {
    //     current_phase++;
    //     object_id = request[current_request].object_id;
    //     for (int i = 1; i <= N; i++)
    //     {
    //         if (i == object[object_id].replica[1])
    //         {
    //             if (current_phase % 2 == 1)
    //             {
    //                 printf("j %d\n", object[object_id].unit[1][current_phase / 2 + 1]);
    //             }
    //             else
    //             {
    //                 printf("r#\n");
    //             }
    //         }
    //         else
    //         {
    //             printf("#\n");
    //         }
    //     }

    //     if (current_phase == object[object_id].size * 2)
    //     {
    //         if (object[object_id].is_delete)
    //         {
    //             printf("0\n");
    //         }
    //         else
    //         {
    //             printf("1\n%d\n", current_request);
    //             request[current_request].is_done = true;
    //         }
    //         current_request = 0;
    //         current_phase = 0;
    //     }
    //     else
    //     {
    //         printf("0\n");
    //     }
    // }

    fflush(stdout);
}

void clean()
{
    for (auto &obj : object)
    {
        for (int i = 1; i <= REP_NUM; i++)
        {
            if (obj.unit[i] == nullptr)
                continue;
            free(obj.unit[i]);
            obj.unit[i] = nullptr;
        }
    }
}

int main()
{
    scanf("%d%d%d%d%d", &T, &M, &N, &V, &G);
    // TODO init 磁盘的空闲链表等等等
    for (auto &D : disks)
    {
        D.init();
    }

    for (int i = 1; i <= M; i++)
    {
        for (int j = 1; j <= (T - 1) / FRE_PER_SLICING + 1; j++)
        {
            scanf("%*d");
        }
    }

    for (int i = 1; i <= M; i++)
    {
        for (int j = 1; j <= (T - 1) / FRE_PER_SLICING + 1; j++)
        {
            scanf("%*d");
        }
    }

    for (int i = 1; i <= M; i++)
    {
        for (int j = 1; j <= (T - 1) / FRE_PER_SLICING + 1; j++)
        {
            scanf("%*d");
        }
    }

    printf("OK\n");
    fflush(stdout);

    // for (int i = 1; i <= N; i++) {
    //     disk_point[i] = 1;
    // }

    for (int t = 1; t <= T + EXTRA_TIME; t++)
    {
        timestamp_action();
        delete_action();
        write_action();
        read_action();
    }
    clean();

    return 0;
}