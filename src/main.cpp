#include "log.h"
#include <thread>
#include <chrono>

// 模拟一个协程ID生成器
uint32_t generateFiberId() {
    static uint32_t fiberId = 0;
    return ++fiberId;
}

// 模拟一个线程ID生成器
uint32_t generateThreadId() {
    static thread_local uint32_t threadId = 0;
    if (threadId == 0) {
        threadId = std::hash<std::thread::id>{}(std::this_thread::get_id());
    }
    return threadId;
}

// 模拟一个线程名称生成器
std::string generateThreadName() {
    static thread_local std::string threadName;
    if (threadName.empty()) {
        threadName = "Thread-" + std::to_string(generateThreadId());
    }
    return threadName;
}

// 测试日志系统
void testLogger() {
    // 创建一个日志器
    auto logger = std::make_shared<zhao::Logger>("TestLogger");

    // 创建一个控制台输出器
    auto consoleAppender = std::make_shared<zhao::ConsoleAppender>();
    consoleAppender->setFormatter(std::make_shared<zhao::LogFormatter>("%d{%Y-%m-%d %H:%M:%S} [%p] [%c] %f:%l %m%n"));
    logger->addDestion(consoleAppender);

    // 创建一个文件输出器
    auto fileAppender = std::make_shared<zhao::FileAppender>("test.log");
    fileAppender->setFormatter(std::make_shared<zhao::LogFormatter>("%d{%Y-%m-%d %H:%M:%S} [%p] [%c] %f:%l %m%n"));
    logger->addDestion(fileAppender);

    // 模拟日志事件
    auto event = std::make_shared<zhao::LogEvent>(
        logger, zhao::LogLevel::INFO, __FILE__, __LINE__, 1000,
        generateThreadId(), generateFiberId(), std::chrono::system_clock::now().time_since_epoch().count(),
        generateThreadName()
    );
    event->m_content = "This is a test log message.";

    // 输出日志
    logger->info(event, consoleAppender);
    logger->debug(event, consoleAppender);
    logger->warn(event, consoleAppender);
    logger->error(event, consoleAppender);
    logger->fatal(event, consoleAppender);

    // 强制输出日志
    logger->log_force(zhao::LogLevel::INFO, event, fileAppender);
}

int main() {
    // 测试日志系统
    testLogger();

    // 多线程测试
    std::thread t1(testLogger);
    //std::thread t2(testLogger);
    t1.join();
    //t2.join();

    return 0;
}