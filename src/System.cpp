// System.cpp
#include "System.hpp"

using namespace std;

RequestManager *requestManager = RequestManager::getInstance();
ObjectManager *objectManager = ObjectManager::getInstance();

// 构造函数参数对应T M N V G

System::System(int TimeStampNum, int TagNum, int DiskNum, int DiskVolume, int TokenG)
    : TimeStampNum(TimeStampNum),
      TagNum(TagNum),
      DiskNum(DiskNum),
      DiskVolume(DiskVolume),
      TokenG(TokenG)
{
    DiskManager::getInstance(DiskNum, DiskVolume, TokenG);
}

// 获取单例实例的静态方法
System &System::getInstance(int TimeStampNum, int TagNum, int DiskNum, int DiskVolume, int TokenG)
{
    static System instance = System(TimeStampNum, TagNum, DiskNum, DiskVolume, TokenG);
    return instance;
}

void System::run()
{
    for (int i = 1; i <= TagNum; i++)
    {
        for (int j = 1; j <= (TimeStampNum - 1) / FRE_PER_SLICING + 1; j++)
        {
            scanf("%*d");
        }
    }
    for (int i = 1; i <= TagNum; i++)
    {
        for (int j = 1; j <= (TimeStampNum - 1) / FRE_PER_SLICING + 1; j++)
        {
            scanf("%*d");
        }
    }
    for (int i = 1; i <= TagNum; i++)
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
        write_action();
        read_action();
        update_time();
        phase_end();
    }
}

void System::timestamp_action()
{
    int timestamp;
    scanf("%*s%d", &timestamp);
    printf("TIMESTAMP %d\n", timestamp);
    fflush(stdout);
}
void System::update_time()
{
    // 更新时间，更新子组件
}

void System::delete_action()
{
    DiskManager DM = DiskManager::getInstance();
    int n_delete;
    static int _id[MAX_OBJECT_NUM];
    // vector<Request> &requests = RequestManager::getInstance()->getRequests();
    // vector<Object> &objects = ObjectManager::getInstance()->getObjects();
    scanf("%d", &n_delete);
    for (int i = 1; i <= n_delete; i++)
    {
        scanf("%d", &_id[i]);
    }
    for (int i = 1; i <= n_delete; i++)
    {
        int id = _id[i];
        DM.remove_obj(id);
    }

    printf("%d\n", DM.canceled_reqs.size());
    for (int canceled_req_id : DM.canceled_reqs)
    {
        printf("%d\n", canceled_req_id);
        // int id = _id[i];
        // int current_id = objects[id].last_request_point;
        // while (current_id != 0)
        // {
        //     if (requests[current_id].is_done == false)
        //     {
        //         printf("%d\n", current_id);
        //     }
        //     current_id = requests[current_id].prev_id;
        // }
        // for (int j = 1; j <= REP_NUM; j++)
        // {
        //     DiskManager::getInstance().store_obj(objects[id].replica[j], *objects[id].unit[j], objects[id].size);
        // }
        // objects[id].is_delete = true;
    }

    fflush(stdout);
}
// 读取请求 添加到scheduler中 scheduler根据负载情况具体交给disk
void System::read_action()
{
    Scheduler &SD = Scheduler::getInstance();
    int n_read;
    int request_id, object_id;
    // vector<Request> &requests = RequestManager::getInstance()->getRequests();
    // vector<Object> &objects = ObjectManager::getInstance()->getObjects();
    scanf("%d", &n_read);
    for (int i = 0; i < n_read; i++)
    {
        scanf("%d%d", &request_id, &object_id);
        // 生成requst
        SD.add_request(request_id, object_id);
        // requests[request_id].object_id = object_id;
        // requests[request_id].prev_id = objects[object_id].last_request_point;
        // objects[object_id].last_request_point = request_id;
        // requests[request_id].is_done = false;
        // Scheduler::getInstance()->del_request(current_id);
    }
    // 找完后输出？
    SD.req_upload();
    fflush(stdout);
}

void System::write_action()
{
    DiskManager &DM = DiskManager::getInstance();
    int n_write;
    scanf("%d", &n_write);
    for (int i = 0; i < n_write; i++)
    {
        int id, size, tag;
        scanf("%d%d%d", &id, &size, &tag);
        DM.store_obj(id, size, tag);
        // int *data = static_cast<int *>(malloc(sizeof(int) * (size + 1)));
        // for (int k = 1; k <= size; k++)
        // {
        //     scanf("%d", &data[k]);
        // }

        // 选择3个磁盘存储副本
        // vector<int> selected_disks = select_disks_for_replica(id);
        // for (int j = 0; j < selected_disks.size(); j++)
        // {
        //     DiskManager::getInstance().store(id, id, size); // 直接调用diskManager的store方法
        // }

        printf("%d\n", id);
        vector<Disk *> disks = DM.get_disks(DM.map_obj_diskid[id]);
        for (int j = 0; j < 3; j++)
        {
            Disk &disk = *disks[j];
            //磁盘id
            printf("%d", disk.id);
            for (int k = 0; k < size; k++)
            {
                //各块地址
                printf(" %d", disk.map_obj_part_addr[id][k]);
            }
            printf("\n");
        }
    }

    fflush(stdout);
}

void System::phase_end()
{
    DiskManager::getInstance().end();
}
