#include "Replica.hpp"
// #include "Unit.hpp"
// Replica::Replica(int id, int size, int tag) : id(id), size(size), tag(tag)
// {
//     // 生成副本时，直接有各单元
//     // Units.resize(size);
//     // for (int part = 0; part < size; part++)
//     // {
//     //     Units[part].part = part;
//     //     Units[part].obj_id = id;
//     // }
//     // parts.resize(size);
// }

// Replica::~Replica()
// {
// }

// std::vector<int> &Replica::getPart()
// {
//     return parts;
// }
// Unit *Replica::getUnit(int part)
// {
//     if (part > size || part < 0)
//         throw std::out_of_range("Part index out of range");
//     return &Units[part];
// }
// std::vector<Unit *> &Replica::getUnits()
// {
//     return Units;
// }

// int Replica::getId() const
// {
//     return id;
// }

// int Replica::getSize() const
// {
//     return size;
// }