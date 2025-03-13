// System.cpp
#include "System.hpp"

using namespace std;

// 初始化静态实例指针为 nullptr
System* System::instance = nullptr;

// 私有构造函数
System::System(int T, int M, int N, int V, int G)
    : TimeStampNum(T), Tags(M), DiskNum(N), DiskVolume(V), Token(G), diskManager(T, M, N, V, G)
{
}

// 获取单例实例的静态方法
System* System::getInstance(int T, int M, int N, int V, int G)
{
    if (instance == nullptr)
    {
        instance = new System(T, M, N, V, G);
    }
    return instance;
}

void System::run()
{
    for (int i = 1; i <= Tags; i++)
    {
        for (int j = 1; j <= (TimeStampNum - 1) / FRE_PER_SLICING + 1; j++)
        {
            scanf("%*d");
        }
    }
    for (int i = 1; i <= Tags; i++)
    {
        for (int j = 1; j <= (TimeStampNum - 1) / FRE_PER_SLICING + 1; j++)
        {
            scanf("%*d");
        }
    }
    for (int i = 1; i <= Tags; i++)
    {
        for (int j = 1; j <= (TimeStampNum - 1) / FRE_PER_SLICING + 1; j++)
        {
            scanf("%*d");
        }
    }
    printf("OK\n");
    fflush(stdout);

    for (int t = 1; t <= TimeStampNum + EXTRA_TIME; t++)
    {
        timestamp_action();
        delete_action();
        write_action(); // TODO: 调用新的写函数
        read_action();
    }
    diskManager.clean();
}

void System::timestamp_action()
{
    int timestamp;
    scanf("%*s%d", &timestamp);
    printf("TIMESTAMP %d\n", timestamp);
    fflush(stdout);
}

void System::delete_action()
{
    int n_delete;
    int abort_num = 0;
    static int _id[MAX_OBJECT_NUM];
    vector<Request>& requests = RequestManager::getInstance()->getRequests();
    vector<Object>& objects = ObjectManager::getInstance()->getObjects();
    scanf("%d", &n_delete);
    for (int i = 1; i <= n_delete; i++)
    {
        scanf("%d", &_id[i]);
    }

    for (int i = 1; i <= n_delete; i++)
    {
        int id = _id[i];
        int current_id = objects[id].last_request_point;

        while (current_id != 0)
        {
            if (requests[current_id].is_done == false)
            {
                Scheduler::getInstance()->del_request(current_id);
                abort_num++;
            }
            current_id = requests[current_id].prev_id;
        }
    }

    printf("%d\n", abort_num);
    for (int i = 1; i <= n_delete; i++)
    {
        int id = _id[i];
        int current_id = objects[id].last_request_point;
        while (current_id != 0)
        {
            if (requests[current_id].is_done == false)
            {
                printf("%d\n", current_id);
            }
            current_id = requests[current_id].prev_id;
        }
        for (int j = 1; j <= REP_NUM; j++)
        {
            diskManager.store(objects[id].replica[j], *objects[id].unit[j], objects[id].size);
        }
        objects[id].is_delete = true;
    }

    fflush(stdout);
}

void System::read_action()
{
    int n_read;
    int request_id, object_id;
    vector<Request>& requests = RequestManager::getInstance()->getRequests();
    vector<Object>& objects = ObjectManager::getInstance()->getObjects();
    scanf("%d", &n_read);
    for (int i = 1; i <= n_read; i++)
    {
        scanf("%d%d", &request_id, &object_id);
        requests[request_id].object_id = object_id;
        requests[request_id].prev_id = objects[object_id].last_request_point;
        objects[object_id].last_request_point = request_id;
        requests[request_id].is_done = false;
        // Scheduler::getInstance()->del_request(current_id);
    }

    for (int i = 1; i <= DiskNum; i++)
    {
        disks[i].task(Scheduler::getInstance()->get_task_for_disk(i), i);
    }
    Scheduler::getInstance()->req_upload();

    fflush(stdout);
}

void System::write_action() {
    int n_write;
    scanf("%d", &n_write);
    for (int i = 1; i <= n_write; i++)
    {
        int id, size;
        scanf("%d%d%*d", &id, &size);
        int* data = static_cast<int*>(malloc(sizeof(int) * (size + 1)));
        for (int k = 1; k <= size; k++) {
            scanf("%d", &data[k]);
        }

        // 选择3个磁盘存储副本
        vector<int> selected_disks = select_disks_for_replica(id);
        for (int j = 0; j < selected_disks.size(); j++) {
            diskManager.store(id, id, size); // 直接调用diskManager的store方法
        }

        printf("%d\n", id);
        for (int j = 0; j < selected_disks.size(); j++)
        {
            printf("%d", selected_disks[j]);
            for (int k = 1; k <= size; k++)
            {
                printf(" %d", data[k]);
            }
            printf("\n");
        }
        free(data);
    }

    fflush(stdout);
}

// 新增: 选择3个磁盘存储副本
vector<int> System::select_disks_for_replica(int id) {
    // TODO: 实现选择3个磁盘存储副本的逻辑
    vector<int> selected_disks;
    // 示例逻辑：随机选择3个磁盘
    for (int i = 0; i < 3; i++) {
        selected_disks.push_back(rand() % DiskNum + 1);
    }
    return selected_disks;
}
