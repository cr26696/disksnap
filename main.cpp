#include <cstdio>
#include <cassert>
#include <cstdlib>

#include <vector>
#include <string>
#include <algorithm>
#include <unordered_set>

using namespace std;

#define MAX_DISK_NUM (10 + 1)
#define MAX_DISK_SIZE (16384 + 1)
#define MAX_REQUEST_NUM (30000000 + 1)
#define MAX_OBJECT_NUM (100000 + 1)
#define REP_NUM (3)
#define FRE_PER_SLICING (1800)
#define EXTRA_TIME (105)

typedef struct Request_
{
    int object_id;
    int prev_id;
    bool is_done;
} Request;

typedef struct Object_
{
    int replica[REP_NUM + 1]; // 第几个副本存放于哪个盘 注意这里索引 起止 1 2 3
    int *unit[REP_NUM + 1];   // 数组，存放对象的某个副本具体存放的子单元在硬盘中的地址
    int size;
    int last_request_point; // 最后一次请求任务
    bool is_delete;
} Object;

Request request[MAX_REQUEST_NUM];
Object object[MAX_OBJECT_NUM];

int T, M, N, V, G;                     // 时间片T 标签M 硬盘N 存储单元数V 磁头tokenG
int disk[MAX_DISK_NUM][MAX_DISK_SIZE]; // 磁盘内容
int disk_point[MAX_DISK_NUM];          // 磁头位置
enum DiskOp
{
    PASS,
    JUMP,
    READ,
};
class Disk
{
    int head = 1;    // 初始为1
    int head_s = -1; // 磁头上个操作 -1初始化 0刚jump过 >0
    // 64刚read过 52=ceil(64*0.8) ... 16 上次连续read过
    // 1 pass过
    int size;
    int blocks[MAX_DISK_SIZE]; // 考虑这里就存 obj_id吗，需不需要其他信息？
    int token;                 // 当前时间片总可用token数
    int elapsed = 0;           // 当前时间片已用token数
    int phase_end = false;

private:
    // 会移动head JUMP需传入跳转地址 PASS传入距离 READ无视参数可填0
    // 返回操作是否进行了操作
    bool operate(DiskOp op, int param)
    {
        switch (op)
        {
        case JUMP:
            if (elapsed > 0)
            {
                phase_end = true;
                return false;
            }
            printf("j %d\n", param);
            head = param;
            head_s = 0;
            elapsed = token;
            phase_end = true;
            break;
        case PASS:
            // 如果给的距离超过了当前时间能移动的上限，会尽量移动（这样算优化吗？）
            if (param >= 1)
            {
                if (elapsed + param > token)
                {
                    // operate(PASS,token-elapsed)//直接移动最大距离？
                    phase_end = true;
                    return false; // 或者调用end？
                }
                string s = string(param, 'p');
                printf("%s", s.c_str()); // 使用 c_str() 将 std::string 转换为 const char*
                head += param;
                head = (head - 1) % size + 1;
                head_s = 1;
                elapsed += param > 2 ? param : 1;
            } // pass 0等于不做动作
            break;
        case READ:
            int consume_token = 64;
            if (head_s > 16)
                consume_token = head_s;

            if (elapsed + consume_token > token)
            {
                phase_end = true;
                return false;
            }
            else
            {
                elapsed += consume_token;
                head_s = consume_token;
                printf("r");
            }
            break;
        default:
            break;
        }
        return true;
    }
    void op_end()
    {
        printf("#\n");
        phase_end = false;
    }

public:
    void init()
    {
        head = 1;
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

    // 传入需要读取的 磁盘的块 的索引
    // 返回 已读取的块 的索引
    vector<int> task(vector<int> target)
    {
        // TODO 调用最优化使用磁头token的算法
        // 第一次出现的目标大于(令牌-64)就进行jump 否则pass至目标 进行读取、
        int next; // 使用to_finds[next]可用得到下一个目标的磁盘地址
        int tokens = G;
        vector<int> found;
        for (int i = 1; i < target.size(); i++)
        {
            if (target[i] >= head)
            {
                next = i;
                break;
            }
        }
        // to_finds[next]是磁头后 最近的目标块
        // 距离 target[next]-head)
        if (target[next] - head + 64 > token)
        {
            // 过远的情况
            operate(JUMP, target[next]);
        }
        else
        {
            while (elapsed + (target[next] - head) < token) // elapsed+距离 < token上限
            {
                int read_consume;
                if (operate(PASS, target[next] - head)) // token足够移动
                {
                    if (operate(READ, 0)) // token足够读取
                    {
                        found.push_back(target[next]); // 放入找到的块
                        if (next + 1 == target.size()) // 尝试找下一个
                            break;
                        next++; // 还有其他要找的情况
                        continue;
                    }
                    else
                        break;
                }
                else
                    break;
            }
        }
        op_end();
        return found;
    }
};

// TODO 任务调度器 读取当前任务列表，计算出待查找块，调用disk.task 具体操作磁头读取
// 收集返回结果  统计已完成任务，最终统一上报
class Scheduler
{
public:
    unordered_set<int> active_requests;
private:
public:
    // 成功失败
    bool add_request(int req_id)
    {
        active_requests.insert(req_id);
    }
    bool del_request(int req_id){
        active_requests.erase(req_id);
    }
    vector<int> get_task_for_disk(int disk_id)
    {
        // TODO 获取一个对应块的数组
        vector<int> target;
        for (int req_id : active_requests)
        {
            Request req = request[req_id];
            Object obj = object[req.object_id];
            for (int i=1;i<=3;i++)
            {
                if (obj.replica[i] == disk_id)
                {
                    int* units = obj.unit[i];
                    for(int i=1;i<=5;i++){
                        target.push_back(units[i]);
                    }
                    break;
                }
            }
        }
        sort(target.begin(),target.end());
        return target;
    }
};
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
            if (request[current_id].is_done == false)
            {
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
    }

    static int current_request = 0;
    static int current_phase = 0;
    if (!current_request && n_read > 0)
    {
        current_request = request_id;
    }
    if (!current_request)
    {
        for (int i = 1; i <= N; i++)
        {
            printf("#\n");
        }
        printf("0\n");
    }
    else
    {
        current_phase++;
        object_id = request[current_request].object_id;
        for (int i = 1; i <= N; i++)
        {
            if (i == object[object_id].replica[1])
            {
                if (current_phase % 2 == 1)
                {
                    printf("j %d\n", object[object_id].unit[1][current_phase / 2 + 1]);
                }
                else
                {
                    printf("r#\n");
                }
            }
            else
            {
                printf("#\n");
            }
        }

        if (current_phase == object[object_id].size * 2)
        {
            if (object[object_id].is_delete)
            {
                printf("0\n");
            }
            else
            {
                printf("1\n%d\n", current_request);
                request[current_request].is_done = true;
            }
            current_request = 0;
            current_phase = 0;
        }
        else
        {
            printf("0\n");
        }
    }

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
    // TODO init 磁盘 各种信息 token 空闲链表等等等
    for (Disk D : disks)
    {
        D.init();
    }
    Scheduler scheduler;
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