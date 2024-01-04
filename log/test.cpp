#include <iostream>
#include <vector>
#include <thread>
//#include "log.h"
#include "/media/mzy/learn_TinyWebServer/log/log.h"

Log* logger = Log::get_instance();
int m_close_log = 0;
void log_test(Log* logger, int thread_id) {
    for (int i = 0; i < 5; ++i) {
        LOG_DEBUG("Thread %d - Debug log message %d", thread_id, i);
        LOG_INFO("Thread %d - Info log message %d", thread_id, i);
        LOG_WARN("Thread %d - Warning log message %d", thread_id, i);
        LOG_ERROR("Thread %d - Error log message %d", thread_id, i);

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

int main() {
    const char* log_file = "example.log";
    
    // logger->log;
    // 初始化日志系统
    if (logger->init(log_file, 1024, 1000, 100)) {
        std::cout << "Log initialization successful." << std::endl;
    } else {
        std::cerr << "Log initialization failed." << std::endl;
        return -1;
    }

    // 启动多线程进行日志测试
    std::vector<std::thread> threads;
    for (int i = 0; i < 3; ++i) {
        threads.emplace_back(log_test, logger, i + 1);
    }

    // 等待所有线程完成
    for (auto& thread : threads) {
        thread.join();
    }

    // 强制刷新日志缓冲
    logger->flush();

    return 0;
}
