#include <thread>
#include <fstream>
#include <chrono>
#include <iostream>
#include <ctime>

#if defined(WIN32)
#   include <windows.h>
#   include <stdlib.h>
#   define MEMORY_NAME "Local\\memory_name"
#   define INVALID_MEMORY (NULL)
#   define SEM_TYPE HANDLE
#else
#   include <unistd.h>
#   include <fcntl.h>
#   include <utility>
#   include <filesystem>
#   include <semaphore.h>
#   include <sys/mman.h>
#   define MEMORY_NAME "/memory_name"
#   define INVALID_MEMORY (-1)
#   define SEM_TYPE sem_t *
#   define HANDLE int
#endif

typedef struct SharedMemoryContent{
    int counter;
    int number_of_opened_programs;
} SharedMemoryContent;

class ThirdLab{
public:
    explicit ThirdLab(std::string curr_fldr);
    void startProgram(int c_i = 0);
    ~ThirdLab();
private:
#if defined(WIN32)
    std::string executable_name;
#endif
    std::atomic<bool> is_threads_continue = true;
    SEM_TYPE semaphore = nullptr;
    std::fstream log_file;
    std::string current_folder;


    HANDLE memory_for_counter;
    bool is_new = false;
    SharedMemoryContent *content;

    void increaseCounter();
    void writeToLogFile();
    void spawnCopies();
    void modifyFromTerminal();

    void createSharedMemory();
    void mapCounterToMemory();
    void unmapCounterToMemory();
    void openSharedMemory();
    void deleteSharedMemory();
    void closeSharedMemory();
    void waitSemaphore();
    void releaseSemaphore();
};
