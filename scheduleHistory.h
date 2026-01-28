// scheduleHistory.h头文件
// 功能说明：管理排班历史记录，保存每次制表时的状态，支持回退到任意历史记录

#pragma once
#include <QString>
#include <QDateTime>
#include <QList>
#include <vector>
#include "Flag_group.h"
#include "dataFunction.h"

// 排班表位置信息（保存Person的标识信息而非指针）
struct SchedulePosition {
    QString personName;      // 队员姓名
    bool personGender;       // 队员性别
    QString personClass;     // 队员班级
    
    SchedulePosition() : personGender(false) {}
    SchedulePosition(const Person* person) {
        if (person) {
            personName = QString::fromStdString(person->getName());
            personGender = person->getGender();
            personClass = QString::fromStdString(person->getClassname());
        }
    }
    
    // 根据标识信息查找Person（在flagGroup中）
    Person* findPerson(Flag_group& flagGroup) const {
        Person temp;
        temp.setName(personName.toStdString());
        temp.setGender(personGender);
        temp.setClassname(personClass.toStdString());
        return flagGroup.findPerson(temp);
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
    
public:
    ScheduleHistoryManager(int maxCount = 50) : maxHistoryCount(maxCount) {}
    
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
