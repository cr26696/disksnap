#ifndef REPLICA_HPP
#define REPLICA_HPP

#include <vector>

class Replica {
private:
    int id;
    std::vector<int> data;
    int size;

public:
    Replica(int id, int* data, int size);
    int getId() const;
    const std::vector<int>& getData() const;
    int getSize() const;
};

#endif // REPLICA_HPP