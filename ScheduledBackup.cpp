// ScheduledBackup.cpp

#include "ScheduledBackup.h"
#include "BackupManager.h"
#include "FilesystemBackupStrategy.h"
#include "InteractiveBackup.h" // 包含 performEncryptedBackupNonInteractive
#include <iostream>
#include <string>
#include <filesystem>
#include <chrono>
#include <thread>
#include <sstream>
#include <iomanip>
#include <ctime>

namespace fs = std::filesystem;


static std::string getInputPath(const std::string& prompt) {
    std::string path;
    while (true) {
        std::cout << prompt;
        std::getline(std::cin, path);
        if (!path.empty()) {
            // Windows 路径兼容处理
#ifdef _WIN32
            for (auto& c : path) if (c == '/') c = '\\';
#endif
            if (fs::exists(path)) {
                return path;
            }
            else {
                std::cout << "路径不存在，请重新输入\n";
            }
        }
        else {
            std::cout << "路径不能为空.\n";
        }
    }
}

static std::string getNewDirectoryPath(const std::string& prompt) {
    std::string path;
    std::cout << prompt;
    std::getline(std::cin, path);
    if (path.empty()) {
        std::cout << "使用默认路径：./backup\n";
        path = "./backup";
    }
#ifdef _WIN32
    for (auto& c : path) if (c == '/') c = '\\';
#endif
    return path;
}

static std::string securePasswordInput(const std::string& prompt) {
    std::string pwd;
    std::cout << prompt;
    std::getline(std::cin, pwd);
    return pwd; 
}

// ----------------------------
// ScheduledBackup 实现
// ----------------------------

ScheduledBackup::ScheduledBackup(
    const std::string& sourceDir,
    const std::string& backupDir,
    bool encrypted,
    const std::string& password,
    const std::string& algorithm)
    : m_sourceDir(sourceDir), m_backupDir(backupDir),
    m_encrypted(encrypted), m_password(password), m_algorithm(algorithm) {
}

ScheduledBackup::ScheduledBackup()
{
}

void ScheduledBackup::start(IntervalType type, int customMinutes) {
    if (m_running) return;

    std::chrono::minutes interval;
    switch (type) {
    case IntervalType::HOURLY: interval = std::chrono::minutes(60); break;
    case IntervalType::DAILY:  interval = std::chrono::minutes(1440); break;
    case IntervalType::CUSTOM_MINUTES:
        interval = (customMinutes > 0) ? std::chrono::minutes(customMinutes)
            : std::chrono::minutes(10);
        break;
    }

    m_running = true;
    m_thread = std::make_unique<std::thread>(&ScheduledBackup::runLoop, this, interval);
}

void ScheduledBackup::stop() {
    m_running = false;
    if (m_thread && m_thread->joinable()) {
        m_thread->join();
    }
}

ScheduledBackup::~ScheduledBackup() {
    stop();
}

void ScheduledBackup::runLoop(std::chrono::minutes interval) {
    while (m_running) {
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);

        // 使用线程安全的本地时间转换（MSVC 用 localtime_s，其他平台用 localtime_r）
        std::tm local_tm;
    #if defined(_MSC_VER)
        localtime_s(&local_tm, &time_t);
    #else
        localtime_r(&time_t, &local_tm);
    #endif

        std::cout << "\n[" << std::put_time(&local_tm, "%Y-%m-%d %H:%M:%S")
            << "] Starting scheduled backup...\n";

        bool success = false;
        if (m_encrypted) {
            // 使用栈上对象，避免 new 导致的内存泄漏
            FilesystemBackupStrategy strategy;
            BackupManager manager(&strategy);
            success = performEncryptedBackupNonInteractive(
                manager,
                m_sourceDir, m_backupDir, m_password, m_algorithm
            );
            // 注：BackupManager 在非交互式加密中未实际使用，可后续优化
        }
        else {
            FilesystemBackupStrategy strategy;
            BackupManager manager(&strategy);
            success = manager.performBackup(m_sourceDir, m_backupDir);
        }

        if (success) {
            std::cout << " 定时备份成功。\n";
        }
        else {
            std::cerr << " 定时备份失败。\n";
        }

        for (int i = 0; i < interval.count() && m_running; ++i) {
            std::this_thread::sleep_for(std::chrono::minutes(1));
        }
    }
}

// ----------------------------
// 静态入口：完整交互流程
// ----------------------------

void ScheduledBackup::runInteractive() {
    std::cout << "\n--- 定时备份设置 ---\n";

    // 1. 获取源目录（必须存在）
    auto source = getInputPath("请输入要备份的源目录：");

    // 2. 获取目标目录（可不存在，将自动创建）
    auto dest = getNewDirectoryPath("请输入备份目标目录（直接按回车使用默认路径 ./backup）：");

    // 3. 是否启用加密
    std::cout << "是否启用加密？(y/n)：";
    std::string enc;
    std::getline(std::cin, enc);
    bool encrypted = (enc == "y" || enc == "Y");

    std::string password, algo = "AES-GCM";
    if (encrypted) {
        password = securePasswordInput("请输入定时备份的密码：");
        if (password.empty()) {
            std::cout << "密码为必填项。已中止定时备份。\n";
            return;
        }
        std::cout << "请选择加密算法：\n1. AES-GCM\n2. ChaCha20-Poly1305\n选择：";
        std::string a;
        std::getline(std::cin, a);
        if (a == "2") algo = "ChaCha20-Poly1305";
    }

    // 4. 创建调度器
    ScheduledBackup scheduler(source, dest, encrypted, password, algo);

    // 5. 设置周期
    std::cout << "请选择备份间隔：\n1. 每小时\n2. 每天\n3. 自定义（分钟）\n选择：";
    std::string itv;
    std::getline(std::cin, itv);

    if (itv == "1") {
        scheduler.start(IntervalType::HOURLY);
    }
    else if (itv == "2") {
        scheduler.start(IntervalType::DAILY);
    }
    else if (itv == "3") {
        std::cout << "请输入间隔时间（分钟）：";
        std::string minStr;
        std::getline(std::cin, minStr);
        int mins = 10;
        try {
            mins = std::stoi(minStr);
        }
        catch (...) {
            mins = 10;
        }
        scheduler.start(IntervalType::CUSTOM_MINUTES, mins);
    }
    else {
        scheduler.start(IntervalType::HOURLY);
    }

    // 6. 等待用户停止
    std::cout << "\n定时备份正在运行中。按下回车键即可停止并返回主菜单...\n";
    std::cin.get(); // 等待回车
    // 析构时自动 stop()
}

// 新增：实例方法 startInteractive 的定义，供 main.cpp 中的调用使用
void ScheduledBackup::startInteractive() {
    ScheduledBackup::runInteractive();
}