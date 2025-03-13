// DiskManager.cpp
#include "DiskManager.hpp"

using namespace std;

DiskManager* DiskManager::instance = nullptr;
DiskManager* DiskManager:: getInstance() {
    if (instance == nullptr) {
        DiskManager::instance = new DiskManager(); // 懒加载创建实例
    }
    return DiskManager::instance;
}

std::vector<Disk>& DiskManager:: getDisks(){
    return DiskManager::disks;
}

void DiskManager::clean()
{
    vector<Object>& objects = ObjectManager::getInstance()->getObjects();
    for (auto &obj : objects)
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

// 修改存储函数实现
// void DiskManager::store(int id, int tag, int size) {
//     // TODO: 分析所有磁盘的存储情况，选择最适合的磁盘
//     vector<Disk*> selected_disks;
//     for (int i = 0; i < N && selected_disks.size() < REP_NUM; i++) {
//         if (disks[i].hasSpace(size)) {
//             selected_disks.push_back(&disks[i]);
//         }
//     }

//     if (selected_disks.size() < REP_NUM) {
//         printf("Failed to find enough disks to store replica %d\n", id);
//         return;
//     }

//     for (int i = 0; i < REP_NUM; i++) {
//         selected_disks[i]->store(id, tag, size); // 调用最适合的磁盘的store方法
//     }
// }

void DiskManager::store(int id, int tag, int size){
    vector<Object>& objects = ObjectManager::getInstance()->getObjects();
    objects[id].last_request_point = 0;
    for (int j = 1; j <= REP_NUM; j++)
    {
        objects[id].replica[j] = (id + j) % N + 1;
        objects[id].unit[j] = static_cast<int *>(malloc(sizeof(int) * (size + 1)));
        objects[id].size = size;
        objects[id].is_delete = false;
        disks[(id + j) % N + 1].write_obj(id, objects[id].unit[j], size);
        // do_object_write(object[id].unit[j], disk[object[id].replica[j]], size, id);
    }

    printf("%d\n", id);
    for (int j = 1; j <= REP_NUM; j++)
    {
        printf("%d", objects[id].replica[j]);
        for (int k = 1; k <= size; k++)
        {
            printf(" %d", objects[id].unit[j][k]);
        }
        printf("\n");
    }
    fflush(stdout); 
}