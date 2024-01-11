#include "ThirdLab.h"
#include <string>
#include <regex>
#define SEMAPHORE_NAME "semaphore_name"

void ThirdLab::createSharedMemory() {
#if defined(WIN32)
    this->memory_for_counter = CreateFileMapping(INVALID_HANDLE_VALUE, nullptr, PAGE_READWRITE, 0, sizeof(SharedMemoryContent), MEMORY_NAME);
#else
    this->memory_for_counter = shm_open(MEMORY_NAME, O_CREAT | O_EXCL | O_RDWR, 0644);
    ftruncate(this->memory_for_counter, sizeof(SharedMemoryContent));
    this->semaphore = sem_open(SEMAPHORE_NAME, O_CREAT | O_EXCL | O_RDWR, 0644);
#endif
}

void ThirdLab::openSharedMemory() {
#if defined(WIN32)
    this->memory_for_counter = OpenFileMapping(FILE_MAP_WRITE, true, MEMORY_NAME);
    this->semaphore = OpenSemaphore(SEMAPHORE_ALL_ACCESS, false, SEMAPHORE_NAME);
#else
    this->memory_for_counter = shm_open(MEMORY_NAME, O_RDWR, 0644);
    this->semaphore = sem_open(SEMAPHORE_NAME, O_RDWR, 0644);
#endif
}

void ThirdLab::closeSharedMemory() {
    this->unmapCounterToMemory();
#if defined(WIN32)
    CloseHandle(this->memory_for_counter);
#else
    close(this->memory_for_counter);
#endif
}

void ThirdLab::deleteSharedMemory() {
    this->closeSharedMemory();
#if defined(WIN32)
#else
    shm_unlink(MEMORY_NAME);
#endif
}

void ThirdLab::unmapCounterToMemory() {
#if defined(WIN32)
    UnmapViewOfFile(this->content);
#else
    munmap(this->content, sizeof(SharedMemoryContent));
#endif
}

void ThirdLab::mapCounterToMemory() {
#if defined(WIN32)
    this->content = reinterpret_cast<SharedMemoryContent*>(MapViewOfFile(this->memory_for_counter, FILE_MAP_WRITE, 0, 0, sizeof(SharedMemoryContent )));
#else
    void *res = mmap(nullptr, sizeof(SharedMemoryContent), PROT_WRITE | PROT_READ, MAP_SHARED, this->memory_for_counter, 0);
    this->content = reinterpret_cast<SharedMemoryContent*>(res);
#endif
    if (this->is_new){
        this->content->counter = 0;
        this->content->number_of_opened_programs = 0;
    }
    this->content->number_of_opened_programs++;
}

void ThirdLab::waitSemaphore() {
#if defined(WIN32)
    WaitForSingleObject(this->semaphore, 0);
#else
    sem_wait(this->semaphore);
#endif
}

void ThirdLab::releaseSemaphore() {
#if defined (WIN32)
    ReleaseSemaphore(this->semaphore, 1, nullptr);
#else
    sem_post(this->semaphore);
#endif
}

ThirdLab::ThirdLab(std::string curr_fldr) {
#if defined(WIN32)
    char ptr_path[MAX_PATH];
    char ptr_file_name[MAX_PATH];
    _splitpath(curr_fldr.c_str(), nullptr, ptr_path, ptr_file_name, nullptr);

    this->executable_name = ptr_file_name;
    this->current_folder = ptr_path;
#else
    this->current_folder = std::filesystem::path(curr_fldr).parent_path();
#endif
    this->openSharedMemory();
    if (this->memory_for_counter == INVALID_MEMORY) {
        this->is_new = true;
        this->createSharedMemory();
    }
    this->mapCounterToMemory();
    this->log_file.open(this->current_folder+"/logfile.txt",  std::fstream::out | std::fstream::app | std::fstream::in);
}

ThirdLab::~ThirdLab(){
    this->content->number_of_opened_programs--;
    if (this->content->number_of_opened_programs == 0){
        this->deleteSharedMemory();
    }
#if defined(WIN32)
    CloseHandle(this->semaphore);
#else
    sem_close(this->semaphore);
#endif
    this->log_file.close();
}

void ThirdLab::increaseCounter() {
    while (this->is_threads_continue) {
        std::this_thread::sleep_for(std::chrono::milliseconds(300));
        this->waitSemaphore();
        this->content->counter++;
        this->releaseSemaphore();
    }
}

void ThirdLab::writeToLogFile() {
    std::cout <<this->log_file.is_open() << std::endl;
    while (this->is_threads_continue) {
        std::this_thread::sleep_for(std::chrono::seconds (1));
        std::time_t end_time = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        if (this->log_file.is_open()) {
            this->waitSemaphore();
            this->log_file << "PID:" << getpid() << std::endl << "Current time: " << std::ctime(&end_time) << " Counter: " << this->content->counter<< std::endl;
            this->releaseSemaphore();
        }
    }
}

void ThirdLab::spawnCopies() {
    while (this->is_threads_continue) {
        std::this_thread::sleep_for(std::chrono::seconds(3));
#if defined(WIN32)
        STARTUPINFO si;
        PROCESS_INFORMATION pi;
        STARTUPINFO si2;
        PROCESS_INFORMATION pi2;
        ZeroMemory( &si, sizeof(si) );
        ZeroMemory( &pi, sizeof(pi) );
        ZeroMemory( &si2, sizeof(si2) );
        ZeroMemory( &pi2, sizeof(pi2) );
        std::string exe_path = this->current_folder + this->executable_name + std::string(".exe");
        std::string cmd_args =exe_path + " COPY1";
        std::string cmd_args2 = exe_path + " COPY2";
        if (!CreateProcess(
                exe_path.c_str(),
                (LPSTR)cmd_args.c_str(),
                NULL,
                NULL,
                FALSE,
                0,
                NULL,
                NULL,
                &si,
                &pi
                )) {
            std::cout << "COPY1 COULDNT BE STARTED" << std::endl;
        }
        if (!CreateProcess(
                exe_path.c_str(),
                (LPSTR)cmd_args2.c_str(),
                NULL,
                NULL,
                FALSE,
                0,
                NULL,
                NULL,
                &si2,
                &pi2
        )) {
            std::cout << "COPY2 COULDNT BE STARTED" << std::endl;
        }
        WaitForSingleObject( pi.hProcess, INFINITE );
        WaitForSingleObject( pi2.hProcess, INFINITE );
        CloseHandle( pi.hProcess );
        CloseHandle( pi.hThread );
        CloseHandle( pi2.hProcess );
        CloseHandle( pi2.hThread );

#else
        int status;
        HANDLE fd[2];
        HANDLE fd_2[2];
        pipe(fd_2);
        pipe(fd);
        pid_t child_pid1 = fork();
        pid_t child_pid2 = 0;

        if (child_pid1) {
            close(fd[0]);
            write(fd[1], &child_pid1, sizeof(int));
            close(fd[1]);
            child_pid2 = fork();
        } else {
            close(fd[1]);
            read(fd[0], &child_pid1, sizeof(int));
            close(fd[0]);
        }
        if (child_pid2) {
            close(fd_2[0]);
            write(fd_2[1], &child_pid2, sizeof(int));
            close(fd_2[1]);
        } else if (getpid() != child_pid1) {
            close(fd_2[1]);
            read(fd_2[0], &child_pid2, sizeof(int));
            close(fd_2[0]);
        }

        if (getpid() == child_pid1) {
            std::time_t end_time = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
            this->waitSemaphore();
            this->log_file << "COPY 1 PID : " << getpid() << " TIME OF START: " << std::ctime(&end_time) << std::endl;
            this->content->counter += 10;
            end_time = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
            this->log_file << "TIME OF END 1 COPY : " << std::ctime(&end_time) << std::endl;
            this->releaseSemaphore();
            return;
        }
        if (getpid() == child_pid2) {
            std::time_t end_time = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
            this->waitSemaphore();
            this->log_file << "COPY 2 PID : " << getpid() << " TIME OF START: " << std::ctime(&end_time) << std::endl;
            this->content->counter *= 10;
            this->releaseSemaphore();

            sleep(2);

            this->waitSemaphore();
            this->content->counter /=10;
            end_time = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
            this->log_file << "TIME OF END 2 COPY : " << std::ctime(&end_time) << std::endl;
            this->releaseSemaphore();
            return;
        }

        waitpid(child_pid1, &status, WUNTRACED);
#endif
    }
}


void ThirdLab::modifyFromTerminal() {
    while ( this->is_threads_continue ){
        int buff;
        std::cin >> buff;
        if (buff == -1){
            this->is_threads_continue = false;
            return;
        }
        this->waitSemaphore();
        this->content->counter = buff;
        this->releaseSemaphore();
    }
}


void ThirdLab::startProgram(int c_i) {
    if (c_i == 1){
        std::time_t end_time = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        this->waitSemaphore();
        this->log_file << "COPY 1 PID : " << getpid() << " TIME OF START: " << std::ctime(&end_time) << std::endl;
        this->content->counter += 10;
        end_time = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        this->log_file << "TIME OF END 1 COPY : " << std::ctime(&end_time) << std::endl;
        this->releaseSemaphore();
        return;
    }
    if (c_i == 2){
        std::time_t end_time = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        this->waitSemaphore();
        this->log_file << "COPY 2 PID : " << getpid() << " TIME OF START: " << std::ctime(&end_time) << std::endl;
        this->content->counter *= 10;
        this->releaseSemaphore();

        std::this_thread::sleep_for(std::chrono::seconds (2));

        this->waitSemaphore();
        this->content->counter /=10;
        end_time = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        this->log_file << "TIME OF END 2 COPY : " << std::ctime(&end_time) << std::endl;
        this->releaseSemaphore();
        return;
    }
    std::thread th_counter(&ThirdLab::increaseCounter, this);
    std::thread th_modify(&ThirdLab::modifyFromTerminal, this);
    std::thread *th_write_log = nullptr;
    std::thread *th_spawn_child = nullptr;

    if (this->is_new) {
        std::time_t end_time = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        if (this->log_file.is_open()) {
            this->waitSemaphore();
            this->log_file << "PID:" << getpid() << std::endl << "Start time: " << std::ctime(&end_time) << std::endl;
            this->releaseSemaphore();
        }
        th_write_log = new std::thread(&ThirdLab::writeToLogFile, this);
        th_spawn_child = new std::thread(&ThirdLab::spawnCopies, this);
    }
    th_counter.join();
    th_modify.join();
    if (th_spawn_child != nullptr) {
        th_write_log->join();
        th_spawn_child->join();
    }
}