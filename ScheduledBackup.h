// ScheduledBackup.h

#pragma once
#include <string>
#include <thread>
#include <atomic>
#include <memory>

class BackupManager; // 前向声明

class ScheduledBackup {
public:
    enum class IntervalType {
        HOURLY,
        DAILY,
        CUSTOM_MINUTES
    };

    ScheduledBackup(
        const std::string& sourceDir,
        const std::string& backupDir,
        bool encrypted,
        const std::string& password = "",
        const std::string& algorithm = "AES-GCM"
    );

    // 添加默认构造函数声明
    ScheduledBackup();

    void start(IntervalType type, int customMinutes = 0);
    void stop();
    ~ScheduledBackup();

    // 新增：完整交互式入口（替代 main.cpp 中的冗长逻辑）
    static void runInteractive();
    void startInteractive();

private:
    void runLoop(std::chrono::minutes interval);

    std::string m_sourceDir;
    std::string m_backupDir;
    bool m_encrypted;
    std::string m_password;
    std::string m_algorithm;

    std::atomic<bool> m_running{ false };
    std::unique_ptr<std::thread> m_thread;
   
};