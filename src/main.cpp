#include <cstdio>
#include <cassert>
#include <cstdlib>
#include <fstream>
#include <filesystem>

#include "MetaDefine.hpp"
#include "System.hpp"
using namespace std;
namespace fs=std::filesystem;

void ensureFileDoesNotExist(const char* filename) {
    fs::path filePath(filename);
    if (fs::exists(filePath)) {
        // 文件存在，删除文件
        if (!fs::remove(filePath)) {
            perror("Failed to delete file");
            exit(EXIT_FAILURE);
        }
    }
}
int main()
{
    const char* logFilename = "source_info.txt";
    ensureFileDoesNotExist(logFilename);
    FILE* logFile = fopen(logFilename, "w");
    if (logFile == nullptr) {
        perror("Failed to create log file");
        exit(EXIT_FAILURE);
    }
    fclose(logFile);

    int T, M, N, V, G;
    scanf("%d%d%d%d%d", &T, &M, &N, &V, &G);
    System &system = System::getInstance(T, M, N, V, G);
    system.run();

    return 0;
}