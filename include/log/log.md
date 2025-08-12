# Log

## 概述

这是一个功能完整的C++日志模块，提供了灵活的日志记录功能，支持模块化日志管理、多级别日志记录、自定义日志格式等功能。

## 核心组件

### 1. Logger（日志记录器）

`zhao::Logger` 是日志系统的核心类，负责管理日志记录的各个方面：

- 支持多级别的日志记录（DEBUG, INFO, WARN, ERROR, FATAL）
- 支持模块化日志管理
- 可以添加多个日志输出目标（Console）
- 支持自定义日志格式

### 2. LogLevel（日志级别）

定义了5个日志级别：

- `DEBUG`：调试信息
- `INFO`：一般信息
- `WARN`：警告信息
- `ERROR`：错误信息
- `FATAL`：致命错误信息

### 3. LogEvent（日志事件）

包含一次日志记录的所有相关信息：

- 文件名和行号
- 线程ID和协程ID
- 时间戳
- 线程名称
- 日志内容流

### 4. LogConsole（日志输出目标）

定义日志的输出方式，可以是控制台、文件等。

## 使用方法

### 基本用法

```
cpp// 创建Logger实例
auto logger = std::make_shared<zhao::Logger>("my_logger");

// 记录日志
LOG_DEBUG(logger) << "This is a debug message";
LOG_INFO(logger) << "This is an info message";
LOG_WARN(logger) << "This is a warning message";
LOG_ERROR(logger) << "This is an error message";
LOG_FATAL(logger) << "This is a fatal error message";
```

### 模块化日志

```
cpp#define MODULE_NAME "network"
MODULE_DEBUG(MODULE_NAME, logger) << "Network connection established";
MODULE_ERROR(MODULE_NAME, logger) << "Failed to connect to server";
```

### 日志格式

默认日志格式：

```
time:%d||logger:[%c]||module:[%M]||dbg:[%p]||tid:%t||threadname:%N||fiberid:%F||file:[%f]||line:%l||content:{%n%m%n}
```

输出示例：

```
time:2025-07-29 12:05:52||logger:[root]||module:[root]||dbg:[DEBUG]||tid:1||threadname:testThread||fiberid:2||file:[src/test/log_test.cpp]||line:8||content:{
hello world
}
```

## 特性

1. **线程安全**：支持多线程环境下的日志记录
2. **模块化管理**：可以针对不同模块设置不同的日志级别
3. **灵活输出**：支持多种输出目标（控制台、文件等）
4. **高性能**：使用智能指针和流式接口，提高日志记录效率
5. **可扩展**：易于添加新的日志输出目标和格式化方式

## 配置和管理

可以通过 [Logger](javascript:void(0)) 类的方法进行配置：

- [addModule()](javascript:void(0))：添加模块
- [modifyModule()](javascript:void(0))：修改模块日志级别
- [maskModule()](javascript:void(0))：屏蔽特定模块
- [addConsole()](javascript:void(0))：添加日志输出目标

这个日志模块为C++应用程序提供了完整的日志记录解决方案，既满足基本的日志记录需求，又具备足够的灵活性来适应复杂的日志管理场景。