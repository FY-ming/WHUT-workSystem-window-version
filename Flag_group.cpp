#include "Flag_group.h"

// 添加队员到指定组
void Flag_group::addPersonToGroup(const Person &person, int groupNumber)
{
    // 参数：Person类：待添加的队员信息。 int groupNumber：队员对应的组别。
    // 根据组别将新队员person加入到对应的组中
    qDebug() << "[Flag_group::addPersonToGroup] 开始添加队员";
    qDebug() << "  - 队员姓名:" << QString::fromStdString(person.getName());
    qDebug() << "  - 目标组别:" << groupNumber;
    qDebug() << "  - 队员自身组别属性:" << person.getGroup();
    
    if (groupNumber >= 1 && groupNumber <= 4) // 判断组号是否合理：1~4对应一至四组
    {
        // 因为group索引最小为0，与输入组号存在一位的差距，需要减一处理
        qDebug() << "  - 添加前，组" << groupNumber << "的队员数量:" << group[groupNumber - 1].size();
        group[groupNumber - 1].push_back(person);// push_back,添加新队员至对应组
        qDebug() << "  - 添加后，组" << groupNumber << "的队员数量:" << group[groupNumber - 1].size();
        qDebug() << "  - 添加成功！";
    }
    else
    {
        qDebug() << "  - 错误：非法组号" << groupNumber;
    }
}

// 从指定组中删除指定的队员
void Flag_group::removePersonFromGroup(const Person &person, int groupNumber)
{
    // 参数：Person类：待删除的队员信息。 int groupNumber：队员对应的组别。
    // 将根据参数的groupNumber在对应的组中查找是否存在参数中的person，查找到后，删除队员。
    qDebug() << "[Flag_group::removePersonFromGroup] 开始删除队员";
    qDebug() << "  - 队员姓名:" << QString::fromStdString(person.getName());
    qDebug() << "  - 目标组别参数 groupNumber:" << groupNumber;
    qDebug() << "  - 队员自身组别属性 person.getGroup():" << person.getGroup();
    
    // 关键检查：如果队员的组别属性与目标组别不一致，发出警告
    if (person.getGroup() != groupNumber) {
        qDebug() << "  - *** 警告：队员的组别属性与目标组别不一致！***";
        qDebug() << "  - *** 这可能导致错误的删除操作！***";
    }
    
    if (groupNumber >= 1 && groupNumber <= 4)
    {
        // 因为group索引最小为0，与输入组号存在一位的差距，需要减一处理
        vector<Person> &currentGroup = group[groupNumber - 1];
        qDebug() << "  - 删除前，组" << groupNumber << "的队员数量:" << currentGroup.size();
        
        // 打印当前组所有队员
        qDebug() << "  - 当前组所有队员:";
        for (size_t i = 0; i < currentGroup.size(); ++i) {
            qDebug() << "    [" << i << "]" << QString::fromStdString(currentGroup[i].getName())
                     << "(组别属性:" << currentGroup[i].getGroup() << ")";
        }
        
        // 从对应组中依次查找是否存在要删除的队员（只通过姓名匹配，确保在指定组内删除）
        bool found = false;
        for (auto it = currentGroup.begin(); it != currentGroup.end(); ++it)
        {
            if (it->getName() == person.getName())
            {
                qDebug() << "  - 找到匹配的队员，准备删除";
                qDebug() << "    - 匹配队员的组别属性:" << it->getGroup();
                currentGroup.erase(it);
                found = true;
                qDebug() << "  - 删除后，组" << groupNumber << "的队员数量:" << currentGroup.size();
                qDebug() << "  - 删除成功！";
                return; // 查找到队员后，终止查找
            }
        }
        
        if (!found) {
            qDebug() << "  - 警告：未找到要删除的队员" << QString::fromStdString(person.getName());
        }
    }
    else
    {
        qDebug() << "  - 错误：非法组号" << groupNumber;
    }
}

// 修改指定组中指定姓名的队员信息
void Flag_group::modifyPersonInGroup(const Person& oldPerson, const Person& newPerson, int groupNumber)
{
    // 参数：Person类：oldPerson:待修改的队员，newPerson：用于替换原队员信息的新信息。 int groupNumber：队员对应的组别。
    // 将根据参数的groupNumber在对应的组中查找是否存在参数中的person，查找到后，用newPerson中数据替换oldPerson的数据，完成修改
    // 修改组别以外的队员信息，如果是组员修改组别信息将不从此函数进行
    qDebug() << "[Flag_group::modifyPersonInGroup] 开始修改队员信息";
    qDebug() << "  - 旧姓名:" << QString::fromStdString(oldPerson.getName());
    qDebug() << "  - 新姓名:" << QString::fromStdString(newPerson.getName());
    qDebug() << "  - 目标组别:" << groupNumber;
    
    if (groupNumber >= 1 && groupNumber <= 4) {
        // 因为group索引最小为0，与输入组号存在一位的差距，需要减一处理
        vector<Person>& currentGroup = group[groupNumber - 1];
        qDebug() << "  - 组" << groupNumber << "的队员数量:" << currentGroup.size();
        
        // 从对应组中依次查找是否存在待修改的队员
        bool found = false;
        for (auto& person : currentGroup) {
            if (person == oldPerson) {
                qDebug() << "  - 找到匹配的队员，准备修改";
                person = newPerson;
                found = true;
                qDebug() << "  - 修改成功！";
                return;
            }
        }
        
        if (!found) {
            qDebug() << "  - 警告：未找到要修改的队员" << QString::fromStdString(oldPerson.getName());
        }
    }
    else
    {
        qDebug() << "  - 错误：非法组号" << groupNumber;
    }

}

// 在指定组中查找指定姓名的队员
Person* Flag_group::findPersonInGroup(const Person &person, int groupNumber)
{
    // 参数：Person类：待查找的队员信息。 int groupNumber：队员对应的组别。
    // 将根据参数的groupNumber在对应的组中查找是否存在参数中的person，查找到后，返回该队员
    if (groupNumber >= 1 && groupNumber <= 4)
    {
        vector<Person> &currentGroup = group[groupNumber - 1];
        for (auto &tempPerson : currentGroup)
        {
            if (tempPerson.getName() == person.getName())
            {
                return &tempPerson;
            }
        }
        // 测试代码
        // std::cerr << "findPersonInGroup()未找到" << person.getName() << endl;
    }
    // 测试代码
    // else
    // {
    //     std::cerr << "非法组号，所属组名应为1~4" << endl;
    // }
    return nullptr;
}

// 在全队查找指定姓名的队员
Person* Flag_group::findPerson(const Person &person) {
    // 参数：Person类：待查找的队员信息。
    for (int i = 1; i <= 4; ++i) {
        Person* found = findPersonInGroup(person, i);
        if (found) {
            return found;
        }
    }
    // 测试代码
    // std::cerr << "findPerson()未找到" << person.getName() << endl;
    return nullptr;
}

// 获取指定组的所有队员
// 非常量版本，允许修改返回的向量
vector<Person>& Flag_group::getGroupMembers(int groupNumber)
{
    // 参数：int groupNumber：待遍历的组别。
    if (groupNumber >= 1 && groupNumber <= 4)
    {
        return group[groupNumber - 1];
    }
    else
    {
        static std::vector<Person> emptyGroup;
        // 测试代码
        // std::cerr << "非法组号，所属组名应为1~4" << std::endl;
        return emptyGroup;
    }
}
// 获取指定组的所有队员
// 常量版本，用于只读访问
const vector<Person>& Flag_group::getGroupMembers(int groupNumber) const
{
    // 参数：int groupNumber：待遍历的组别。
    if (groupNumber >= 1 && groupNumber <= 4)
    {
        return group[groupNumber - 1];
    }
    else
    {
        static vector<Person> emptyGroup;
        // 测试代码
        // std::cerr << "非法组号，所属组名应为1~4" << endl;
        return emptyGroup;
    }
}
// 判断整个国旗班是否为空（所有组都没有成员）
bool Flag_group::isEmpty() const {
    for (int i = 0; i < 4; ++i) {
        if (!group[i].empty()) {
            return false;
        }
    }
    return true;
}
