// dataFunction.h头文件
// 功能说明：主要用于提供执勤工作表的相关数据处理函数

// 有关表格信息的额外说明
// scheduleTable索引全都参照下面的表格结构
// 表格结构大致如下
//          ||        周一         ||        周二         ||        周三        ||        周四         ||        周五         ||
//      |南鉴湖升旗 ||slot:0   location:0 || slot:2   location:0||slot:4   location:0 ||slot:6   location:0 ||slot:8   location:0 ||
// 升旗  --------------------------------------------------------------------------------------------------------------------------
//      |东西院升旗 ||slot:0   location:1 ||slot:2   location:1 ||slot:4   location:1 ||slot:6   location:1 ||slot:8   location:1 ||
// -------------------------------------------------------------------------------------------------------------------------------
//      |南鉴湖降旗 ||slot:1   location:0 ||slot:3   location:0 ||slot:5   location:0 ||slot:7   location:0 ||slot:9   location:0 ||
// 降旗  --------------------------------------------------------------------------------------------------------------------------
//      |东西院降旗 ||slot:1   location:1 ||slot:3   location:1 ||slot:5   location:1 ||slot:7   location:1 ||slot:9   location:1 ||

#pragma once

#include <QObject>
#include <QDebug>
#include <vector>
#include <algorithm>
#include <random>
#include <string>
#include "Person.h"
#include "Flag_group.h"


// SchedulingManager 类定义，执勤工作表
class SchedulingManager : public QObject
{
    Q_OBJECT // QObject宏定义

signals:
    void schedulingWarning(const QString& warningMessage); // 某一时间段无法安排队员执勤时的警告信号
    void schedulingFinished(); // 排表完成后的提示信号

public:
    // 新增的排表模式枚举
    enum class ScheduleMode {
        Normal,           // 常规模式
        Supervisory,      // 监督模式
        DXYMondayFriday,  // 东西院周一升周五降模式
        Custom            // 自定义模式
    };

    // 构造函数
    SchedulingManager(const Flag_group& flagGroup)
        : flagGroup(flagGroup),
        mode(ScheduleMode::Normal)  // 初始化模式为常规模式
    {
        initializeAvailableMembers();// 通过队员的isWork的信息统计参加排班的人
    }


    // 部署工作表基础准备资源，排班操作的入口
    void schedule() {
        const int totalSlots = 10;// 一周10个工作时间段,升旗时间对应0 2 4 6 8
        const int locationsPerSlot = 2;// 两个工作地点（0:南鉴湖、1:东西院）
        const int peoplePerLocation = 3;// 一个工作地点的三名执勤队员
        for (auto& member : availableMembers) {
            // 重置每个参加排班的队员本周的工作次数：0
            member->setTimes(0);
        }

        // 随机数生成装置，生成高质量随机数种子，理论上不可能重复
        std::random_device rd;
        // 创建一个 std::mt19937 类型的随机数引擎 g，并使用 rd() 生成的随机数种子对其进行初始化。
        // std::mt19937 是一个基于梅森旋转算法的伪随机数生成器，能够生成高质量的随机数序列。
        // 理论上存在重复的可能，但运算周期极长，只要随机数种子不同，理论不会重复。
        // 因为是伪随机数，所以种子一样，结果一样
        std::mt19937 g(rd());
        // 调用 std::shuffle 函数，将 availableMembers 向量中的元素顺序随机打乱。
        // std::shuffle 函数接受三个参数：容器的起始迭代器、容器的结束迭代器以及随机数引擎。
        // 借助 std::shuffle 函数，能够将 availableMembers 向量中的队员指针顺序随机打乱。
        // 在分配剩余工作量时，每个队员都有相同的概率获得额外的工作机会，避免了因队员在列表中的初始顺序而导致的不公平现象。
        // 若不使用 std::shuffle 对 availableMembers 进行随机打乱，那么每次剩余工作量都会优先分配给列表前面的队员。
        // 长期下来，这会造成队员之间的工作量不均衡，前面的队员工作次数会明显多于后面的队员。
        std::shuffle(availableMembers.begin(), availableMembers.end(), g);


        // 排班！
        // 对 scheduleTable 进行初始化，它是一个三维向量，用于存储排班结果。
        // totalSlots：表示一周内的总工作时间段数量。在当前的排班规则下，一周工作 5 天，每天分上午和下午两个时间段，所以 totalSlots 为 10。
        // locationsPerSlot：每个工作时间段内的工作地点数量，这里是 2 个（“NJH” 和 “DXY”）。
        // peoplePerLocation：每个工作地点需要的工作人员数量，这里是 3 人。
        // nullptr：初始时，每个排班位置都设置为 nullptr，表示尚未安排人员。
        scheduleTable.resize(totalSlots, std::vector<std::vector<Person*>>(locationsPerSlot, std::vector<Person*>(peoplePerLocation, nullptr)));
        for (int slot = 0; slot < totalSlots; ++slot) {
            //外层循环遍历工作时间段
            int day = slot / 2 + 1;//值为1~5。表示星期
            int halfDay = slot % 2;//值为0~1。0:上午，1：下午
            for (int location = 0; location < locationsPerSlot; ++location) {
                // 中层循环遍历工作地点
                int timeRow = halfDay * 2 + location + 1;//location=0~1,timeRow=1~4，分别表示NJH升旗，DXY升旗，NJH降旗，DXY降旗
                for (int position = 0; position < peoplePerLocation; ++position) {
                    //内层循环遍历工作岗位
                    Person* selectedPerson = selectPerson(slot, timeRow, location, day);//选择合适队员
                    if (selectedPerson) {
                        // 如果找到合适队员，加入工作表格scheduleTable中
                        scheduleTable[slot][location][position] = selectedPerson;
                        selectedPerson->setTimes(selectedPerson->getTimes() + 1);
                        selectedPerson->setAll_times(selectedPerson->getAll_times() + 1);
                    }
                }
            }
        }
        // 发出排班完成信号
        emit schedulingFinished();
        checkedPositions = 0; // 每次结束排表重置checkedPositions避免下次警告信息异常
    }

    // 成员变量的get与set函数声明
    bool getUseTotalTimesRule() const;
    const Flag_group &getFlagGroup() const;
    std::vector<std::vector<std::vector<Person *> > > getScheduleTable() const;
    void setScheduleTable(const std::vector<std::vector<std::vector<Person *> > > &newScheduleTable);
    std::vector<Person *> getAvailableMembers() const;
    void setAvailableMembers(const std::vector<Person *> &newAvailableMembers);
    // 设置排表模式
    void setScheduleMode(ScheduleMode newMode) {
        mode = newMode;
    }


private:
    const Flag_group& flagGroup; // 国旗班容器，保存队员信息
    std::unordered_map<std::string, int> warningCount; // 键值对容器，用于记录交接规则失败警告信息出现的次数
    std::vector<Person*> availableMembers; // 容器，保存参加排班的队员
    std::vector<std::vector<std::vector<Person*>>> scheduleTable; // 工作表格
    bool useTotalTimesRule;
    ScheduleMode mode;
    int checkedPositions = 0;        // 记录所有周二上午南鉴湖岗位的筛选结果已检查的岗位数

    void initializeAvailableMembers() {
        // 初始化辅助函数
        // 通过队员的isWork的信息统计参加排班的人
        for (int group = 1; group <= 4; ++group) {
            const auto& members = flagGroup.getGroupMembers(group);
            for (const auto& member : members) {
                if (member.getIsWork()) {
                    availableMembers.push_back(const_cast<Person*>(&member));
                }
            }
        }
    }
    Person* selectPerson(int slot, int timeRow,int location, int day) {
        // 制表辅助函数
        // 选择合适的可工作队员
        // slot=0~9，表示10个时间段（周一上午、周一下午、周二上午、周二下午…… 周五下午）
        // timeRow=1~4，表格行数，分别表示NJH升旗，DXY升旗，NJH降旗，DXY降旗
        // location=0~1，工作地点，分别表示南鉴湖，东西院
        // day = 1~5, 工作的时间，对应周一至周五
        if(mode != ScheduleMode::Custom)
        {
            // 复制可用人员列表并按总次数排序
            std::vector<Person*> candidates = availableMembers;
            if (candidates.empty()) { // 防御性检查
                emit schedulingWarning("警告：没有可用的候选人员！");
                return nullptr;
            }

            std::sort(candidates.begin(), candidates.end(), [](Person* a, Person* b) {
                return a->getAll_times() < b->getAll_times();
            });

            // 应用不同模式的约束条件
            std::vector<Person*> validCandidates = applyModeConstraints(candidates, slot, timeRow, location, day);
            std::vector<Person*> eligibleCandidates; // 符合交接规则的候选人列表
            const std::vector<Person*>& allValidCandidates = validCandidates; // 保存所有有效候选人指针

            // 仅在周二上午南鉴湖时检查交接规则
            if (slot == 2 && location == 0) {
                eligibleCandidates.reserve(allValidCandidates.size());
                // 筛选符合交接规则的候选人指针
                for (Person* person : allValidCandidates) {
                    if (isPersonSatisfyHandoverRule(person, slot, location)) {
                        eligibleCandidates.push_back(person);
                    }
                }
                // 处理筛选结果
                if (!eligibleCandidates.empty()) {
                    // 从符合交接规则的候选人中随机选择
                    std::random_device rd;
                    std::mt19937 gen(rd());
                    std::uniform_int_distribution<> dis(0, eligibleCandidates.size() - 1);
                    return eligibleCandidates[dis(gen)];
                } else {
                    checkedPositions++;
                    if(checkedPositions >= 3) {
                        // 所有有效候选人都不符合交接规则，发出警告
                        emit schedulingWarning(QString::fromStdString(
                            "警告：周二上午南鉴湖任务中，无法满足交接规则，放弃以确保表格完整。"
                            ));
                    }
                }
                // 无论是否符合交接规则，都从所有有效候选人中随机选择
                // 确保交接规则不会导致某些人负担过重
            }

            // 从所有有效候选人中选择（改进版：更加均衡的次数分配）
            if (allValidCandidates.empty()) {
                // 处理无有效候选人的情况
                if ((mode != ScheduleMode::DXYMondayFriday) || ((location == 1) && (slot == 0 || slot == 9))) {
                    emit schedulingWarning(QString::fromStdString(
                        "警告：在 " + getTimeDescription(slot, location) + " 无法选出合适的人员进行排班。"
                        ));
                }
                return nullptr;
            }

            // 计算当前最小排班次数和最大排班次数
            int minTimes = INT_MAX;
            for (Person* p : allValidCandidates) {
                int times = p->getAll_times();
                if (times < minTimes) minTimes = times;
            }

            // 筛选出排班次数等于minTimes的候选人
            eligibleCandidates.clear();
            for (Person* p : allValidCandidates) {
                if (p->getAll_times() == minTimes) {
                    eligibleCandidates.push_back(p);
                }
            }

            // 如果没有候选人满足条件，退回到使用所有有效候选人
            if (eligibleCandidates.empty()) {
                eligibleCandidates = allValidCandidates;
            }

            // 在符合条件的候选人中随机选择
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_int_distribution<> dis(0, eligibleCandidates.size() - 1);
            return eligibleCandidates[dis(gen)];
        }
        return nullptr;
    }
    std::string getTimeDescription(int slot, int location) {
        std::string days[] = { "周一", "周二", "周三", "周四", "周五" };
        std::string halves[] = { "上午升旗", "下午降旗" };
        std::string locations[] = { "南鉴湖", "东西院" };

        int dayIndex = slot / 2;
        int halfDayIndex = slot % 2;

        std::string timeDesc = days[dayIndex] + locations[location] + halves[halfDayIndex] ;
        return timeDesc;
    }

    std::vector<Person*> applyModeConstraints(const std::vector<Person*>& candidates, int slot, int timeRow, int location, int day) {
        std::vector<Person*> validCandidates;

        for (auto person : candidates) {
            bool isValid = true;

            // 基础条件：时间有空且未被安排
            if (!person->getTime(timeRow, day) || isPersonBusy(person, slot)) {
                isValid = false;
            }

            // 应用女队员限制规则
            if (isValid && wouldExceedFemaleLimit(person, slot, location)) {
                isValid = false;
            }

            // 应用监督模式规则（至少一名非大一队员）
            if (isValid && mode == ScheduleMode::Supervisory) {
                if (!isSupervisoryRequirementMet(person, slot, location)) {
                    isValid = false;
                }
            }

            // 应用东西院模式规则（仅特定位置排班）
            if (isValid && mode == ScheduleMode::DXYMondayFriday) {
                if (!isValidSlotForDXYMode(slot, location)) {
                    isValid = false;
                }
            }

            if (isValid) {
                validCandidates.push_back(person);
            }
        }

        return validCandidates;
    }

    bool wouldExceedFemaleLimit(Person* person, int slot, int location) {
        if (person->getGender() != true) {
            return false;
        }

        // 计算当前任务中已有的女队员数量
        int femaleCount = 0;
        for (int pos = 0; pos < 3; ++pos) {
            if (scheduleTable[slot][location][pos] &&
                scheduleTable[slot][location][pos]->getGender() == true) {
                femaleCount++;
            }
        }

        // 如果加入当前队员会使女队员数量达到3，则返回true
        return (femaleCount + 1) >= 3;
    }

    bool isPersonSatisfyHandoverRule(Person* person, int slot, int location) {
        // 检查周二交接规则：slot=2, location=0的任务中，至少有一人参与了slot=1, location=0的任务
        if (slot != 2 || location != 0) {
            return true; // 不是周二交接的任务，直接返回true
        }

        // 检查slot=1, location=0的任务中是否有当前人员
        for (int pos = 0; pos < 3; ++pos) {
            if (scheduleTable[1][0][pos] == person) {
                return true;
            }
        }

        return false;
    }

    bool isSupervisoryRequirementMet(Person* person, int slot, int location) {
        // 如果当前人员是非大一学生，直接满足条件
        if (person->getGrade() != 1) {
            return true;
        }

        // 如果当前人员是大一学生，检查当前任务中是否已经有非大一学生
        for (int pos = 0; pos < 3; ++pos) {
            if (scheduleTable[slot][location][pos] &&
                scheduleTable[slot][location][pos]->getGrade() != 1) {
                return true;
            }
        }

        // 如果当前任务中没有非大一学生，且候选人员是大一学生，则不满足条件
        return false;
    }

    bool isValidSlotForDXYMode(int slot, int location) {
        // DXYMondayFriday模式只对特定位置进行排班
        // slot:0 location:0 || slot:2 location:0||slot:4 location:0 ||slot:6 location:0 ||slot:8 location:0
        // || slot:0 location:1 ||
        // || slot:1 location:0 ||slot:3 location:0 ||slot:5 location:0 ||slot:7 location:0 ||slot:9 location:0
        // || slot:9 location:1 ||

        bool isValidNJH = (location == 0);

        bool isValidDXY = (location == 1) && (slot == 0 || slot == 9);

        return isValidNJH || isValidDXY;
    }


    // 判断是否已经在同一时间段安排了工作
    bool isPersonBusy(Person* person, int slot) const {
        // 将对应时间段slot的所有位置都遍历一遍，查看是否已经存在该队员person
        for (int location = 0; location < 2; ++location) {
            for (int position = 0; position < 3; ++position) {
                if (scheduleTable[slot][location][position] == person) {
                    return true;
                }
            }
        }
        return false;
    }
};



inline std::vector<std::vector<std::vector<Person *> > > SchedulingManager::getScheduleTable() const
{
    return scheduleTable;
}

inline void SchedulingManager::setScheduleTable(const std::vector<std::vector<std::vector<Person *> > > &newScheduleTable)
{
    scheduleTable = newScheduleTable;
}

inline std::vector<Person *> SchedulingManager::getAvailableMembers() const
{
    return availableMembers;
}

inline void SchedulingManager::setAvailableMembers(const std::vector<Person *> &newAvailableMembers)
{
    availableMembers = newAvailableMembers;
}


