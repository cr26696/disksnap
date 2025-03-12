// Scheduler.cpp
#include "Scheduler.hpp"

bool Scheduler::add_request(int req_id)
{
    int object_id = request[req_id].object_id;
    active_requests[object_id].push_back(req_id);
    return true; // 假设添加总是成功
}

bool Scheduler::del_request(int req_id)
{
    int object_id = request[req_id].object_id;
    auto& req_list = active_requests[object_id];
    auto it = std::find(req_list.begin(), req_list.end(), req_id);
    if (it != req_list.end())
    {
        req_list.erase(it);
        if (req_list.empty())
        {
            active_requests.erase(object_id);
        }
        return true;
    }
    return false;
}

std::vector<int> Scheduler::get_task_for_disk(int disk_id)
{
    std::vector<int> target;

    for (const auto& [object_id, req_ids] : active_requests)
    {
        Request req = request[req_ids[0]]; // 假设第一个请求代表整个对象
        Object obj = object[req.object_id];

        for (int i = 1; i <= 3; i++)
        {
            if (obj.replica[i] == disk_id)
            {
                int *units = obj.unit[i];
                for (int j = 1; j <= obj.size; j++) // 修改变量名避免冲突
                {
                    target.push_back(units[j]);
                }
                break;
            }
        }
    }
    std::sort(target.begin(), target.end());
    DEBUG_PRINT(target.size());
    return target;
}

void Scheduler::req_upload()
{
    int complete_num = 0;
    std::string info = "";
    bool complete_flag;

    for (const auto& [object_id, req_ids] : active_requests)
    {
        for (int actreq_id : req_ids)
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
    }
    info = std::to_string(complete_num) + "\n" + info; // 转换complete_num为字符串
    std::cout << info;
}