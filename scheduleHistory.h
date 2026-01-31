// scheduleHistory.h头文件
// 功能说明：管理排班历史记录，保存每次制表时的状态，支持回退到任意历史记录
// 持久化：历史仅内存，写入 ./data/schedule_history.dat 仅在退出且用户未选「不保存」时进行；
// 若用户选「不保存」，本次会话的增删改全部丢弃，文件保持启动时状态。

#pragma once
#include <QString>
#include <QDateTime>
#include <QList>
#include <QDataStream>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QDebug>
#include <vector>
#include "Flag_group.h"
#include "dataFunction.h"

// 排班表位置信息（保存Person的标识信息而非指针）
struct SchedulePosition {
    QString personName;      // 队员姓名
    int personGroup;         // 队员组别（用于精确定位）
    
    SchedulePosition() : personGroup(0) {}
    SchedulePosition(const Person* person) {
        if (person) {
            personName = QString::fromStdString(person->getName());
            personGroup = person->getGroup();
        }
    }
    
    // 根据标识信息查找Person（在flagGroup中）
    // 使用姓名+组别查找（确保在指定组内唯一）
    Person* findPerson(Flag_group& flagGroup) const {
        if (personGroup >= 1 && personGroup <= 4 && !personName.isEmpty()) {
            Person temp;
            temp.setName(personName.toStdString());
            return flagGroup.findPersonInGroup(temp, personGroup);
        }
        return nullptr;
    }
};

// 历史记录项结构
struct ScheduleHistoryItem {
    QDateTime timestamp;              // 制表时间
    QString mode;                    // 排班模式（常规模式、监督模式等）
    Flag_group flagGroupSnapshot;    // 队员信息快照
    std::vector<std::vector<std::vector<SchedulePosition>>> scheduleTable; // 排班表快照（保存标识信息）
    QString scheduleText;            // 排班结果文本
    int totalMembers;                // 总队员数
    int totalScheduleCount;          // 总排班次数
    
    ScheduleHistoryItem() : totalMembers(0), totalScheduleCount(0) {}
};

class ScheduleHistoryManager
{
private:
    QList<ScheduleHistoryItem> historyList;  // 历史记录列表
    int maxHistoryCount;  // 最大保存历史记录数量（防止占用过多内存）
    QString m_historyFilePath;  // 历史记录文件路径，用于持久化
    
    static bool writeFlagGroupToStream(QDataStream& out, const Flag_group& g) {
        for (int i = 1; i <= 4; ++i) {
            const auto& members = g.getGroupMembers(i);
            out << static_cast<qint32>(members.size());
            for (const auto& p : members) {
                out << QString::fromStdString(p.getName()) << p.getGender()
                    << static_cast<qint32>(p.getGroup()) << static_cast<qint32>(p.getGrade())
                    << QString::fromStdString(p.getPhone_number())
                    << QString::fromStdString(p.getNative_place())
                    << QString::fromStdString(p.getNative())
                    << QString::fromStdString(p.getDorm())
                    << QString::fromStdString(p.getSchool())
                    << QString::fromStdString(p.getClassname())
                    << QString::fromStdString(p.getBirthday())
                    << p.getIsWork();
                for (int r = 1; r <= 4; ++r)
                    for (int c = 1; c <= 5; ++c)
                        out << p.getTime(r, c);
                out << static_cast<qint32>(p.getTimes())
                    << static_cast<qint32>(p.getAll_times())
                    << static_cast<qint32>(p.getNJHAllTimes())
                    << static_cast<qint32>(p.getDXYAllTimes());
            }
        }
        return (out.status() == QDataStream::Ok);
    }
    
    static bool readFlagGroupFromStream(QDataStream& in, Flag_group& g, qint32 version) {
        for (int grp = 1; grp <= 4; ++grp) {
            qint32 n;
            in >> n;
            for (int i = 0; i < n; ++i) {
                qint32 personId = 0;
                // 版本2及以上支持ID（但版本3+已废弃，仅用于兼容读取）
                if (version >= 2 && version < 3) {
                    in >> personId;
                }
                QString name; bool gender; qint32 group, grade;
                QString phone, native_place, native, dorm, school, classname, birthday;
                bool isWork;
                in >> name >> gender >> group >> grade
                   >> phone >> native_place >> native >> dorm >> school >> classname >> birthday
                   >> isWork;
                bool time[4][5];
                for (int r = 0; r < 4; ++r)
                    for (int c = 0; c < 5; ++c)
                        in >> time[r][c];
                qint32 times, all_times, njh = 0, dxy = 0;
                in >> times >> all_times;
                if (version >= 1) in >> njh >> dxy;
                Person person(name.toStdString(), gender, group, grade,
                    phone.toStdString(), native_place.toStdString(), native.toStdString(),
                    dorm.toStdString(), school.toStdString(), classname.toStdString(),
                    birthday.toStdString(), isWork, time, times, all_times, njh, dxy);
                g.addPersonToGroup(person, group);
            }
        }
        return (in.status() == QDataStream::Ok);
    }
    
public:
    ScheduleHistoryManager(int maxCount = 50) : maxHistoryCount(maxCount) {}
    
    void setHistoryFilePath(const QString& path) { m_historyFilePath = path; }
    QString historyFilePath() const { return m_historyFilePath; }
    
    /// 从文件加载历史记录。成功则覆盖当前列表并保存路径；失败则保留原列表。
    bool loadFromFile(const QString& path) {
        QFile f(path);
        if (!f.exists() || !f.open(QIODevice::ReadOnly)) {
            if (f.exists()) qDebug() << "无法打开历史文件：" << path;
            return false;
        }
        if (f.size() == 0) { f.close(); return false; }
        QDataStream in(&f);
        in.setVersion(QDataStream::Qt_5_15);
        QString magic;
        qint32 version, count;
        in >> magic >> version >> count;
        if (magic != "SCHEDULE_HISTORY_V1" || in.status() != QDataStream::Ok) {
            f.close();
            return false;
        }
        QList<ScheduleHistoryItem> tmp;
        for (int k = 0; k < count && in.status() == QDataStream::Ok; ++k) {
            ScheduleHistoryItem item;
            in >> item.timestamp >> item.mode;
            if (!readFlagGroupFromStream(in, item.flagGroupSnapshot, version)) break;
            qint32 d0, d1, d2;
            in >> d0 >> d1 >> d2;
            item.scheduleTable.resize(d0);
            for (int i = 0; i < d0; ++i) {
                item.scheduleTable[i].resize(d1);
                for (int j = 0; j < d1; ++j) {
                    item.scheduleTable[i][j].resize(d2);
                    for (int t = 0; t < d2; ++t) {
                        if (version >= 2 && version < 3) {
                            // 兼容旧版本：读取但忽略ID、性别、班级信息
                            qint32 id;
                            bool gender;
                            QString className;
                            in >> id;
                            in >> item.scheduleTable[i][j][t].personName >> gender >> className;
                            // 无法从旧数据恢复组别，设为0（后续查找时会失败，但不会崩溃）
                            item.scheduleTable[i][j][t].personGroup = 0;
                        } else if (version >= 3) {
                            // 新版本：读取姓名和组别
                            qint32 group;
                            in >> item.scheduleTable[i][j][t].personName >> group;
                            item.scheduleTable[i][j][t].personGroup = group;
                        } else {
                            // 版本1：只有姓名、性别、班级
                            bool gender;
                            QString className;
                            in >> item.scheduleTable[i][j][t].personName >> gender >> className;
                            item.scheduleTable[i][j][t].personGroup = 0;
                        }
                    }
                }
            }
            in >> item.scheduleText >> item.totalMembers >> item.totalScheduleCount;
            if (in.status() == QDataStream::Ok) tmp.append(item);
        }
        f.close();
        if (in.status() != QDataStream::Ok) return false;
        historyList = tmp;
        m_historyFilePath = path;
        return true;
    }
    
    /// 将当前历史记录写入已设置路径的文件。使用临时文件+重命名保证写入原子性。
    bool saveToFile() const {
        if (m_historyFilePath.isEmpty()) return false;
        QString tmpPath = m_historyFilePath + ".tmp";
        QFile tmp(tmpPath);
        if (QFile::exists(tmpPath)) QFile::remove(tmpPath);
        if (!tmp.open(QIODevice::WriteOnly)) {
            qDebug() << "无法创建历史临时文件：" << tmpPath;
            return false;
        }
        QDataStream out(&tmp);
        out.setVersion(QDataStream::Qt_5_15);
        // 版本3：移除队员唯一ID，使用姓名+组别作为唯一标识
        out << QString("SCHEDULE_HISTORY_V1") << static_cast<qint32>(3)
            << static_cast<qint32>(historyList.size());
        for (const auto& item : historyList) {
            out << item.timestamp << item.mode;
            if (!writeFlagGroupToStream(out, item.flagGroupSnapshot)) {
                tmp.close();
                QFile::remove(tmpPath);
                return false;
            }
            qint32 d0 = static_cast<qint32>(item.scheduleTable.size());
            qint32 d1 = d0 ? static_cast<qint32>(item.scheduleTable[0].size()) : 0;
            qint32 d2 = (d0 && d1) ? static_cast<qint32>(item.scheduleTable[0][0].size()) : 0;
            out << d0 << d1 << d2;
            for (const auto& row : item.scheduleTable)
                for (const auto& col : row)
                    for (const auto& pos : col)
                        out << pos.personName << static_cast<qint32>(pos.personGroup);
            out << item.scheduleText << static_cast<qint32>(item.totalMembers)
                << static_cast<qint32>(item.totalScheduleCount);
        }
        tmp.close();
        if (out.status() != QDataStream::Ok) {
            QFile::remove(tmpPath);
            return false;
        }
        if (QFile::exists(m_historyFilePath) && !QFile::remove(m_historyFilePath)) {
            qDebug() << "无法覆盖历史文件：" << m_historyFilePath;
            QFile::remove(tmpPath);
            return false;
        }
        if (!tmp.rename(m_historyFilePath)) {
            qDebug() << "无法重命名历史临时文件为：" << m_historyFilePath;
            QFile::remove(tmpPath);
            return false;
        }
        return true;
    }
    
    // 添加新的历史记录
    void addHistory(const Flag_group& flagGroup, 
                   const SchedulingManager& manager,
                   const QString& mode,
                   const QString& scheduleText) {
        ScheduleHistoryItem item;
        item.timestamp = QDateTime::currentDateTime();
        item.mode = mode;
        item.scheduleText = scheduleText;
        
        // 深拷贝队员信息
        item.flagGroupSnapshot = flagGroup;
        
        // 保存排班表（将Person指针转换为标识信息）
        const auto& scheduleTable = manager.getScheduleTable();
        item.scheduleTable.resize(scheduleTable.size());
        for (size_t slot = 0; slot < scheduleTable.size(); ++slot) {
            item.scheduleTable[slot].resize(scheduleTable[slot].size());
            for (size_t location = 0; location < scheduleTable[slot].size(); ++location) {
                item.scheduleTable[slot][location].resize(scheduleTable[slot][location].size());
                for (size_t position = 0; position < scheduleTable[slot][location].size(); ++position) {
                    const Person* person = scheduleTable[slot][location][position];
                    item.scheduleTable[slot][location][position] = SchedulePosition(person);
                }
            }
        }
        
        // 统计信息
        int totalMembers = 0;
        int totalScheduleCount = 0;
        for (int i = 1; i <= 4; ++i) {
            const auto& members = flagGroup.getGroupMembers(i);
            totalMembers += members.size();
            for (const auto& member : members) {
                totalScheduleCount += member.getAll_times();
            }
        }
        item.totalMembers = totalMembers;
        item.totalScheduleCount = totalScheduleCount;
        
        // 添加到列表
        historyList.append(item);
        
        // 如果超过最大数量，删除最旧的记录
        while (historyList.size() > maxHistoryCount) {
            historyList.removeFirst();
        }
        // 不在此处写盘；历史仅内存，退出时若用户选择「保存」才写入文件
    }
    
    // 获取历史记录列表
    const QList<ScheduleHistoryItem>& getHistoryList() const {
        return historyList;
    }
    
    // 获取指定索引的历史记录
    const ScheduleHistoryItem* getHistory(int index) const {
        if (index >= 0 && index < historyList.size()) {
            return &historyList.at(index);
        }
        return nullptr;
    }
    
    // 获取历史记录数量
    int getHistoryCount() const {
        return historyList.size();
    }
    
    // 清空所有历史记录
    void clearHistory() {
        historyList.clear();
    }
    
    // 删除指定索引的历史记录
    bool removeHistory(int index) {
        if (index >= 0 && index < historyList.size()) {
            historyList.removeAt(index);
            return true;
        }
        return false;
    }
};
