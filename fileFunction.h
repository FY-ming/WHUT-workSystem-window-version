// fileFunction.h头文件
// 功能说明：对数据进行文件读写操作，实现队员信息写入文件，从文件中读取队员信息，从而提升系统的复用性

#pragma once
#include <string>
#include <QFile>
#include <QTextStream>
#include <QDebug>
#include "Flag_group.h"
class FlagGroupFileManager
{
public:
    // 文件写入函数，将队员信息保存至文件中
    static void saveToFile(const Flag_group& flagGroup, const QString& filename) {
        // 参数：Flag_group：存放国旗班所有队员信息的容器。QString filename：文件名
        QFile file(filename); // 创建QFile对象
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) // 检查写入文件是否能访问
        {
            QTextStream out(&file);// 打开数据文件，准备写入
            for (int i = 1; i <= 4; ++i) { // 循环，完成全部四组的队员数据写入
                const auto& members = flagGroup.getGroupMembers(i);// 获取某一组的全部队员信息
                for (const auto& person : members) {// 依次写入某一个队员的全部信息
                    out << QString::fromStdString(person.getName()) << "|" // 姓名
                        << (person.getGender() ? "1" : "0") << "|" // 性别
                        << person.getGroup() << "|" // 所属组别
                        << person.getGrade() << "|"
                        << QString::fromStdString(person.getPhone_number()) << "|" // 联系电话
                        << QString::fromStdString(person.getNative_place()) << "|" // 籍贯
                        << QString::fromStdString(person.getNative()) << "|" // 民族
                        << QString::fromStdString(person.getDorm()) << "|" // 寝室号
                        << QString::fromStdString(person.getSchool()) << "|" // 学院
                        << QString::fromStdString(person.getClassname()) << "|" // 专业班级
                        << QString::fromStdString(person.getBirthday()) << "|" // 生日
                        << (person.getIsWork() ? "1" : "0") << "|"; // 是否参与排班
                    for (int j = 1; j < 5; ++j) {
                        for (int k = 1; k < 6; ++k) {
                            out << (person.getTime(j, k) ? "1" : "0") << "|"; // 时间安排表
                        }
                    }
                    out << person.getTimes() << "|" // 本次执勤次数
                        << person.getAll_times() << "\n"; // 总执勤次数
                }
            }
            file.close();// 关闭文件
        }
        // 测试代码
        // else {// 无法打开文件时的报错处理
        //     qDebug() << "无法打开文件 " << filename << " 进行写入！";
        // }
    }
    // 读取文件函数
    static void loadFromFile(Flag_group& flagGroup, const QString& filename) {
        // 参数：Flag_group容器，QString文件名
        QFile file(filename);
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTextStream in(&file);// 打开数据文件，准备读取
            while (!in.atEnd()) {// 判断文件读取是否结束
                QString line = in.readLine();// 读取一行文件数据
                QStringList parts = line.split("|"); // 选定的数据分隔号
                if (parts.size() == 12 + 20 + 2) {// 判定是否是正确的Person数据类型，如果是空行/其他错误数据，将不进行读取保存。
                    std::string name = parts[0].toStdString(); // 姓名
                    bool gender = parts[1].toInt(); // 性别
                    int group = parts[2].toInt(); // 所属组别
                    int grade = parts[3].toInt(); // 所属年级
                    std::string phone_number = parts[4].toStdString(); // 联系电话
                    std::string native_place = parts[5].toStdString(); // 籍贯
                    std::string native = parts[6].toStdString(); // 民族
                    std::string dorm = parts[7].toStdString(); // 寝室号
                    std::string school = parts[8].toStdString(); // 学院
                    std::string classname = parts[9].toStdString(); // 专业班级
                    std::string birthday = parts[10].toStdString(); // 生日
                    bool isWork = parts[11].toInt(); // 是否参与排班
                    bool time[4][5];
                    for (int i = 0; i < 4; ++i) {
                        for (int j = 0; j < 5; ++j) {
                            time[i][j] = parts[12 + i * 5 + j].toInt(); // 时间安排表
                        }
                    }
                    int times = parts[32].toInt(); // 本次执勤次数
                    int all_times = parts[33].toInt(); // 总执勤次数
                    Person person(name, gender, group, grade, phone_number, native_place, native, dorm, school, classname, birthday, isWork, time, times, all_times);
                    flagGroup.addPersonToGroup(person, group);
                }
            }
            file.close();// 关闭文件
        }
        // 测试代码
        // else {
        //     qDebug() << "无法打开文件 " << filename << " 进行读取！";
        // }
    }
};
