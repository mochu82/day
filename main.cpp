#include <iostream>
#include <string>
#include "FilesystemBackupStrategy.h"
#include "BackupManager.h"
#include "InteractiveBackup.h"
#include "InteractiveRestore.h"
#include "ZipCompressor.h"
#include "ZipExtractor.h"
#include "BatchZipCompressor.h"
#include "BatchZipExtractor.h"
#include "PathUtils.h"
#include "crypto.h"
#include <minizip/zip.h>
#include "metadata.h"
#include <fstream>  // 解决 E0070
#include "ScheduledBackup.h"
#include "BackupValidator.h"  // ← 新增：用于验证
#include <filesystem>

namespace fs = std::filesystem;

/**
 * @brief 主函数
 * 创建用户交互界面，提供备份、还原、加密、解密等功能
 */
int main() {
    std::ios_base::sync_with_stdio(false);
    std::wcout.imbue(std::locale("")); // 使用系统本地 locale（如 zh-CN）
    // 创建备份策略和管理器
    FilesystemBackupStrategy strategy;
    BackupManager manager(&strategy);

    std::cout << "=== 文件备份系统  ===" << std::endl;
    std::cout << "支持压缩、解压、加密、解密、定时备份、还原等功能" << std::endl;

    int choice;
    while (true) {
        // 显示主菜单
        std::cout << "\n=== 主菜单 ===" << std::endl;
        std::cout << "1. 普通备份（不加密）" << std::endl;
        std::cout << "2. 普通还原（不解密）" << std::endl;
        std::cout << "3. 加密备份（压缩+加密）" << std::endl;
        std::cout << "4. 加密还原（解密+解压）" << std::endl;
        std::cout << "5. 仅压缩目录为ZIP" << std::endl;
        std::cout << "6. 仅解压ZIP文件" << std::endl;
        std::cout << "7. 定时备份" << std::endl;
        std::cout << "8. 文件元属性" << std::endl;
        std::cout << "9. 批量打包多个目录为一个ZIP" << std::endl;
        std::cout << "10. 对zip文件进行解包" << std::endl;
        std::cout << "11. 验证备份完整性（比较两个目录）" << std::endl;
        std::cout << "12. 退出程序" << std::endl;
        std::cout << "请选择功能 (1-12): ";

        if (!(std::cin >> choice)) {
            std::cin.clear();
            std::cin.ignore(10000, '\n');
            std::cout << "输入错误，请输入数字 1-12。" << std::endl;
            continue;
        }

        std::cin.ignore();

        switch (choice) {
        case 1: {
            // 普通备份
            PerformBackupInteractive(manager);
            break;
        }

        case 2:
            // 普通还原（直接目录复制）
            PerformRestoreInteractive(manager);
            break;

        case 3: {
            // 加密备份
            performEncryptedBackup(manager); 
            break;
        }

        case 4:
            // 加密还原（已修复：能真正还原文件到目标目录）
            performEncryptedRestore(manager);
            break;

        case 5: {
            std::string src = PathUtils::getUserInput("请输入要压缩的目录路径: ");
            std::string zip = PathUtils::getUserInput("请输入ZIP文件保存路径（如 backup.zip）: ");
            ZipCompressor::compressDirectory(src, zip);
            break;
        }

        case 6: {
            // 仅解压
            std::string zip = PathUtils::getUserInput("请输入ZIP文件路径: ");
            std::string dest = PathUtils::getUserInput("请输入解压目标目录: ");
            if (ZipExtractor::extractToDirectory(zip, dest)) {
                std::cout << "解压成功！\n";
            }
            else {
                std::cout << "解压失败！\n";
            }
            break;
        }

        case 7: {
            // 定时备份
            ScheduledBackup scheduledBackup;
            scheduledBackup.startInteractive();
            break;
        }

        case 12: {
            std::cout << "再见！" << std::endl;
            return 0;
        }
        case 9: {
                  // 批量打包
                  std::vector<std::string> sources;
                  while (true) {
                      std::string path = PathUtils::getUserInput("请输入要打包的目录路径 (或输入 0 结束): ");
                      if (path == "0") break;
                      if (!path.empty()) {
                          sources.push_back(path);
                      }
                  }
                  if (sources.empty()) {
                      std::cout << "未输入任何路径，取消打包。\n";
                      break;
                  }
                  std::string outputZip = PathUtils::getUserInput("请输入输出 ZIP 路径（如 merged.zip）: ");
                  BatchZipCompressor::batchCompress(sources, outputZip);
                  break;
              }
        case 10: {
            std::string zipPath = PathUtils::getUserInput("请输入 ZIP 文件路径: ");
            std::string out1 = PathUtils::getUserInput("请输入第一个输出目录: ");
            std::string out2 = PathUtils::getUserInput("请输入第二个输出目录: ");
            if (BatchZipExtractor::splitExtract(zipPath, out1, out2)) {
                std::cout << "分离解压成功！\n";
            }
            else {
                std::cout << "分离解压失败！\n";
            }
            break;
        }
        case 11: {
            std::string original = PathUtils::getUserInput("请输入原始数据目录路径: ");
            std::string backup = PathUtils::getUserInput("请输入备份目录路径: ");
            if (BackupValidator::validate(original, backup)) {
                std::cout << "备份验证成功：两个目录内容完全一致！\n";
            }
            else {
                std::cout << "备份验证失败：目录内容不一致或路径无效。\n";
            }
            break;
        }
        case 8: {
            std::string path = PathUtils::getUserInput("请输入文件路径: ");
            if (!fs::exists(path)) {
                std::cout << "路径不存在.\n";
                break;
            }
            FileMetadata meta = GetFileMetadata(path);
            std::cout << "\n=== 文件元数据 ===\n";
            meta.print(); 
            std::cout << "=====================\n";
            break;
            
        }


        default:
            std::cout << "无效选择，请输入 1-12。" << std::endl;
        }
    }

    return 0;
}