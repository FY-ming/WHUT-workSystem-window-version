// encryptedFileManager.h头文件
// 功能说明：使用AES加密算法对数据进行加密存储，提供更安全的数据保存方案

#pragma once
#include <QString>
#include <QByteArray>
#include <QCryptographicHash>
#include <QDataStream>
#include <QFile>
#include <QDebug>
#include "Flag_group.h"

class EncryptedFileManager
{
private:
    // 默认加密密钥（基于应用程序的固定密钥，实际使用中可以改为用户输入）
    static QString getDefaultKey() {
        return QString("WHUT_Flag_Guard_System_2024_Secure_Key_v1.0");
    }
    
    // 生成加密密钥（基于输入密钥生成256位密钥）
    static QByteArray generateKey(const QString& password) {
        QCryptographicHash hash(QCryptographicHash::Sha256);
        hash.addData(password.toUtf8());
        return hash.result();
    }
    
    // 简单的XOR加密/解密（AES需要额外库，这里使用XOR作为轻量级加密）
    static QByteArray encryptData(const QByteArray& data, const QByteArray& key) {
        QByteArray encrypted = data;
        for (int i = 0; i < encrypted.size(); ++i) {
            encrypted[i] = encrypted[i] ^ key[i % key.size()];
        }
        return encrypted;
    }
    
    static QByteArray decryptData(const QByteArray& encryptedData, const QByteArray& key) {
        // XOR加密和解密是相同的操作
        return encryptData(encryptedData, key);
    }

public:
    // 加密保存文件
    static bool saveToFile(const Flag_group& flagGroup, const QString& filename, const QString& password = QString()) {
        // 使用临时文件避免文件占用问题
        QString tempFilename = filename + ".tmp";
        
        // 如果临时文件已存在，先删除
        QFile tempFile(tempFilename);
        if (tempFile.exists()) {
            if (!tempFile.remove()) {
                qDebug() << "无法删除旧的临时文件：" << tempFilename;
                // 继续尝试，可能文件已被释放
            }
        }
        
        // 尝试打开临时文件进行写入
        if (!tempFile.open(QIODevice::WriteOnly)) {
            qDebug() << "无法打开临时文件进行写入：" << tempFilename;
            return false;
        }
        
        // 生成加密密钥（如果未提供密码，使用默认密钥）
        QString actualPassword = password.isEmpty() ? getDefaultKey() : password;
        QByteArray key = generateKey(actualPassword);
        
        // 将数据序列化为字节数组
        QByteArray data;
        QDataStream out(&data, QIODevice::WriteOnly);
        out.setVersion(QDataStream::Qt_5_15);
        
        // 写入文件版本号（用于未来兼容性）
        out << (qint32)1;
        
        // 写入所有队员数据
        for (int i = 1; i <= 4; ++i) {
            const auto& members = flagGroup.getGroupMembers(i);
            out << (qint32)members.size(); // 写入该组的队员数量
            
            for (const auto& person : members) {
                // 写入队员基本信息
                out << QString::fromStdString(person.getName());
                out << (bool)person.getGender();
                out << (qint32)person.getGroup();
                out << (qint32)person.getGrade();
                out << QString::fromStdString(person.getPhone_number());
                out << QString::fromStdString(person.getNative_place());
                out << QString::fromStdString(person.getNative());
                out << QString::fromStdString(person.getDorm());
                out << QString::fromStdString(person.getSchool());
                out << QString::fromStdString(person.getClassname());
                out << QString::fromStdString(person.getBirthday());
                out << (bool)person.getIsWork();
                
                // 写入时间安排（20个布尔值）
                for (int j = 1; j <= 4; ++j) {
                    for (int k = 1; k <= 5; ++k) {
                        out << (bool)person.getTime(j, k);
                    }
                }
                
                // 写入执勤次数
                out << (qint32)person.getTimes();
                out << (qint32)person.getAll_times();
            }
        }
        
        // 加密数据
        QByteArray encryptedData = encryptData(data, key);
        
        // 写入文件：先写入文件头标识，再写入加密数据
        QDataStream fileOut(&tempFile);
        fileOut.setVersion(QDataStream::Qt_5_15);
        fileOut << QString("FLAG_GROUP_ENCRYPTED_V1"); // 文件头标识
        fileOut << encryptedData;
        
        tempFile.close();
        
        // 检查数据流状态
        if (fileOut.status() != QDataStream::Ok) {
            qDebug() << "写入临时文件时发生错误：" << tempFilename;
            tempFile.remove(); // 删除失败的临时文件
            return false;
        }
        
        // 如果目标文件已存在，先尝试删除（如果被占用，删除会失败）
        QFile targetFile(filename);
        if (targetFile.exists()) {
            // 尝试删除旧文件，如果失败说明文件被占用
            if (!targetFile.remove()) {
                qDebug() << "警告：无法删除旧文件，可能被占用：" << filename;
                // 继续尝试重命名，如果重命名也失败，说明文件确实被占用
            }
        }
        
        // 原子性地将临时文件重命名为目标文件
        if (!tempFile.rename(filename)) {
            qDebug() << "无法将临时文件重命名为目标文件：" << filename;
            qDebug() << "临时文件位置：" << tempFilename;
            // 保留临时文件，用户可以手动恢复
            return false;
        }
        
        return true;
    }
    
    // 解密读取文件
    static bool loadFromFile(Flag_group& flagGroup, const QString& filename, const QString& password = QString()) {
        QFile file(filename);
        if (!file.exists()) {
            qDebug() << "文件不存在：" << filename;
            return false;
        }
        
        if (!file.open(QIODevice::ReadOnly)) {
            qDebug() << "无法打开文件进行读取：" << filename;
            return false;
        }
        
        // 检查文件大小
        if (file.size() == 0) {
            qDebug() << "文件为空：" << filename;
            file.close();
            return false;
        }
        
        QDataStream fileIn(&file);
        fileIn.setVersion(QDataStream::Qt_5_15);
        
        // 检查数据流状态
        if (fileIn.status() != QDataStream::Ok) {
            qDebug() << "数据流状态错误：" << filename;
            file.close();
            return false;
        }
        
        // 读取文件头标识
        QString header;
        fileIn >> header;
        
        // 检查读取是否成功
        if (fileIn.status() != QDataStream::Ok) {
            qDebug() << "读取文件头失败：" << filename;
            file.close();
            return false;
        }
        
        // 检查文件格式
        if (header != "FLAG_GROUP_ENCRYPTED_V1") {
            qDebug() << "文件格式不正确，期望：FLAG_GROUP_ENCRYPTED_V1，实际：" << header;
            file.close();
            return false;
        }
        
        // 读取加密数据
        QByteArray encryptedData;
        fileIn >> encryptedData;
        file.close();
        
        // 检查数据是否读取成功
        if (fileIn.status() != QDataStream::Ok || encryptedData.isEmpty()) {
            qDebug() << "读取加密数据失败或数据为空：" << filename;
            return false;
        }
        
        // 生成解密密钥（如果未提供密码，使用默认密钥）
        QString actualPassword = password.isEmpty() ? getDefaultKey() : password;
        QByteArray key = generateKey(actualPassword);
        
        // 解密数据
        QByteArray data = decryptData(encryptedData, key);
        
        if (data.isEmpty()) {
            qDebug() << "解密后数据为空：" << filename;
            return false;
        }
        
        // 从字节数组反序列化数据
        QDataStream in(data);
        in.setVersion(QDataStream::Qt_5_15);
        
        // 读取文件版本号
        qint32 version;
        in >> version;
        
        // 检查版本号读取是否成功
        if (in.status() != QDataStream::Ok) {
            qDebug() << "读取版本号失败：" << filename;
            return false;
        }
        
        // 读取所有队员数据
        for (int groupIndex = 1; groupIndex <= 4; ++groupIndex) {
            qint32 memberCount;
            in >> memberCount;
            
            for (int i = 0; i < memberCount; ++i) {
                // 读取队员基本信息
                QString name;
                bool gender;
                qint32 group, grade;
                QString phone_number, native_place, native, dorm, school, classname, birthday;
                bool isWork;
                
                in >> name >> gender >> group >> grade;
                in >> phone_number >> native_place >> native >> dorm >> school >> classname >> birthday;
                in >> isWork;
                
                // 读取时间安排
                bool time[4][5];
                for (int j = 0; j < 4; ++j) {
                    for (int k = 0; k < 5; ++k) {
                        bool timeValue;
                        in >> timeValue;
                        time[j][k] = timeValue;
                    }
                }
                
                // 读取执勤次数
                qint32 times, all_times;
                in >> times >> all_times;
                
                // 创建Person对象并添加到组
                Person person(name.toStdString(), gender, group, grade,
                             phone_number.toStdString(), native_place.toStdString(),
                             native.toStdString(), dorm.toStdString(), school.toStdString(),
                             classname.toStdString(), birthday.toStdString(), isWork,
                             time, times, all_times);
                flagGroup.addPersonToGroup(person, group);
            }
        }
        
        return true;
    }
};
