#include "Replica.hpp"

Replica::Replica(int id, int* data, int size) : id(id), size(size) {
    this->data.assign(data, data + size);
}

int Replica::getId() const {
    return id;
}

const std::vector<int>& Replica::getData() const {
    return data;
}

int Replica::getSize() const {
    return size;
}