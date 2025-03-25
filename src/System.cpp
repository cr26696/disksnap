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
    // DiskManager::getInstance(DiskNum, DiskVolume, TokenG);

    // 计算时间片个数：ceil(TimeStampNum / FRE_PER_SLICING)
    PeriodNum = (TimeStampNum - 1) / FRE_PER_SLICING + 1;
    // 分配连续存储空间：每个数组的大小为 TagNum * numTimeSlices
    fre_del.assign(TagNum, std::vector<int>(PeriodNum, 0));
    fre_write.assign(TagNum, std::vector<int>(PeriodNum, 0));
    fre_read.assign(TagNum, std::vector<int>(PeriodNum, 0));
}

// 获取单例实例的静态方法
System &System::getInstance(int TimeStampNum, int TagNum, int DiskNum, int DiskVolume, int TokenG)
{
    static System instance = System(TimeStampNum, TagNum, DiskNum, DiskVolume, TokenG);
    return instance;
}

void System::run()
{
    for (int i = 0; i < TagNum; i++)
    {
        for (int j = 0; j < PeriodNum; j++)
        {
            scanf("%d", &fre_del[i][j]);
        }
    }
    for (int i = 0; i < TagNum; i++)
    {
        for (int j = 0; j < PeriodNum; j++)
        {
            scanf("%d", &fre_write[i][j]);
        }
    }
    for (int i = 0; i < TagNum; i++)
    {
        for (int j = 0; j < PeriodNum; j++)
        {
            scanf("%d", & fre_read[i][j]);
        }
    }
    printf("OK\n");
    fflush(stdout);

    //TODO变成动态？
    label_oriented_storge();  
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
    DiskManager& DM = DiskManager::getInstance();
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
    DiskManager &DM = DiskManager::getInstance();
    int n_read;
    int request_id, object_id;
    // vector<Request> &requests = RequestManager::getInstance()->getRequests();
    // vector<Object> &objects = ObjectManager::getInstance()->getObjects();
    scanf("%d", &n_read);
    for (int i = 0; i < n_read; i++)
    {
        scanf("%d%d", &request_id, &object_id);
        // 生成requst
        SD.add_request(request_id, object_id);//
        // requests[request_id].object_id = object_id;
        // requests[request_id].prev_id = objects[object_id].last_request_point;
        // objects[object_id].last_request_point = request_id;
        // requests[request_id].is_done = false;
        // Scheduler::getInstance()->del_request(current_id);
    }
    // 找完后输出？
    DM.find();
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
        DM.store_obj(id, size, tag, label_index);

        printf("%d\n", id);
        vector<Disk *> disks = DM.get_disks(DM.map_obj_diskid[id]);
        for (int j = 0; j < 3; j++)
        {
            Disk &disk = *disks[j];
            //磁盘id IMP 输出时地址从1开始
            DEBUG_PRINT(disk.id);
            DEBUG_PRINT(disk.id+1);
            printf("%d", disk.id+1);
            for (int k = 0; k < size; k++)
            {
                //各块地址
                printf(" %d", disk.map_obj_part_addr[id][k]+1);
            }
            printf("\n");
        }
    }

    fflush(stdout);
}

//暂时用写-删，计算分配的存储空间
void System::label_oriented_storge()
{
    std::vector<int> total_del(TagNum, 0);  // 初始化结果向量
    std::vector<int> total_wri(TagNum, 0);
    std::vector<int> total_read(TagNum, 0);
    std::vector<int> total_writes(TagNum, 0);
    std::vector<float> ratio(TagNum, 0.0);
    int writes = 0;
    label_index.resize(TagNum, 0);

    for (int i = 0; i < TagNum; i++) {
        for (int j = 0; j < PeriodNum; j++) {
            total_del[i] += fre_del[i][j];
            total_wri[i] += fre_write[i][j];
            // total_read[i] += fre_read[i][j];
        }
        // total_writes[i] = total_wri[i] - total_del[i];
        writes += total_wri[i];
    }
    assert(writes != 0);

    //有必要按读的热值对标签存储顺序进行排序写入吗？
    for (int i = 0; i < TagNum; i++){
        ratio[i] = (float)total_wri[i] / writes; //TODO 删写想等时分配一块默认子磁盘大小！
        assert(ratio[i] >= 0.0);
        //数据，记录不同标签存储起点
        if (i == 0) 
            label_index[0] = 0;
        else
            label_index[i] = label_index[i-1] + static_cast<int>(ratio[i-1] * DiskVolume); //最多损失TagNum个的空间
    }
    DiskManager::getInstance(DiskNum, DiskVolume, TokenG, label_index);
}

void System::phase_end()
{
    DiskManager::getInstance().end();
}
