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

}

void System::init()
{
    //TODO 读取标签 创建DiskManager对象 创建Disk 
    PeriodNum = (TimeStampNum - 1) / FRE_PER_SLICING + 1;
    fre_del.assign(TagNum, std::vector<int>(PeriodNum, 0));
    fre_write.assign(TagNum, std::vector<int>(PeriodNum, 0));
    fre_read.assign(TagNum, std::vector<int>(PeriodNum, 0));

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

    label_oriented_storge();
    DiskManager::getInstance(DiskNum, DiskVolume, TokenG, tag_ratio);
}

// 获取单例实例的静态方法
System &System::getInstance(int TimeStampNum, int TagNum, int DiskNum, int DiskVolume, int TokenG)
{
    static System instance = System(TimeStampNum, TagNum, DiskNum, DiskVolume, TokenG);
    return instance;
}

void System::run()
{
    init();
    for (int t = 1; t <= TimeStampNum + EXTRA_TIME; t++)
    {
        timestamp_action();
        delete_action();
        write_action();
        read_action(); // TODO 调用persuade_thread使用查找功能
        phase_end();
    }
}

void System::timestamp_action()
{
    int timestamp;
    scanf("%*s%d", &timestamp);
    printf("TIMESTAMP %d\n", timestamp);
    TimeStamp = timestamp;
    fflush(stdout);
}
void System::update_time()
{
    TimeStamp++;
}

void System::delete_action()
{
    Scheduler &SD = Scheduler::getInstance();
    DiskManager &DM = DiskManager::getInstance();
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
    vector<int> canceled_reqs_id = SD.get_canceled_reqs_id();
    int cancel_num = canceled_reqs_id.size();
    printf("%d\n", cancel_num);
    for (int i = 0; i < cancel_num; i++)
    {
        printf("%d\n", canceled_reqs_id[i]);
    }
    fflush(stdout);
}
// 读取请求 添加到scheduler中 scheduler根据负载情况具体交给disk
void System::read_action()
{
    bool mute = false;
    Scheduler &SD = Scheduler::getInstance();
    int n_read;
    int request_id, object_id;
    // vector<Request> &requests = RequestManager::getInstance()->getRequests();
    // vector<Object> &objects = ObjectManager::getInstance()->getObjects();
    scanf("%d", &n_read);
    for (int i = 0; i < n_read; i++)
    {
        scanf("%d%d", &request_id, &object_id);
        SD.add_request(request_id, object_id);
    }
    SD.excute_find();
    // 找完后输出？
    if (!mute)
    {
        SD.req_upload();
    }
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
        Object &obj_info = DM.store_obj(id, size, tag);
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

        // 现在diskmanager的store中已经上报过了
        //  printf("%d\n", id);
        //  // vector<Disk *> disks = DM.get_disks(DM.map_obj_diskid[id]);
        //  for (int j = 0; j < 3; j++)
        //  {
        //      int disk_id = obj_info.diskid_replica[j];
        //      Disk &disk = DM.get_disk(disk_id);
        //      //磁盘ido
        //      printf("%d", disk.id);
        //      for (int k = 0; k < size; k++)
        //      {

        //         printf(" %d", disk.map_obj_part_addr[id][k]);
        //     }
        //     printf("\n");
        // }
    }

    fflush(stdout);
}

void System::label_oriented_storge()
{
    std::vector<int> total_del(TagNum, 0);  // 初始化结果向量
    std::vector<int> total_wri(TagNum, 0);
    std::vector<int> total_read(TagNum, 0);
    std::vector<int> total_writes(TagNum, 0);
    // std::vector<float> ratio(TagNum, 0.0);
    int writes = 0;
    tag_ratio.resize(TagNum, 0);

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

    //TODO 有必要按读的热值对标签存储顺序进行排序写入吗？
    for (int i = 0; i < TagNum; i++){
        tag_ratio[i] = (float)total_wri[i] / writes; //TODO 删写想等时分配一块默认子磁盘大小！
        assert(tag_ratio[i] >= 0.0);
        
        //记录不同标签存储起点
        // if (i == 0) 
        //     label_index[0] = 0;
        // else
        //     label_index[i] = label_index[i-1] + static_cast<int>(tag_ratio[i-1] * DiskVolume);
    }
}

void System::phase_end()
{
    DiskManager::getInstance().end();
    Scheduler::getInstance().end();
}
