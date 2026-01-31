// Flag_group.h头文件
// 功能说明：
// 设计WHUT国旗班Flag_group类，内置一个vector容器数组group，用于存放一到四组所有成员。
// Flag_group类作为容器，将担任对保存所有队员信息、对队员进行增删改查功能的实现等职责

#pragma once
#include<iostream>
#include<vector>
#include<QString>
#include<QDebug>
#include"Person.h"
using std::vector;
using std::endl;

class Flag_group
{
public:
    Flag_group(){}
    //操作group容器的函数
    //如果存在重名情况将对同名者的第一个被检索的人进行操作
    void addPersonToGroup(const Person &person, int groupNumber); // 添加队员到指定组
    void removePersonFromGroup(const Person &person, int groupNumber); // 从指定组中删除指定的队员
    void modifyPersonInGroup(const Person& oldPerson, const Person& newPerson, int groupNumber); // 修改指定组中指定姓名的队员信息
    Person* findPersonInGroup(const Person &person, int groupNumber); // 在指定组中查找指定的队员
    Person* findPerson(const Person &person); // 在全队查找指定姓名的队员
    vector<Person>& getGroupMembers(int groupNumber); // 获取指定组的所有队员，返回可修改引用版本
    const vector<Person>& getGroupMembers(int groupNumber) const; // 获取指定组的所有队员，返回常量版本
    bool isEmpty() const; // 检测容器是否为空
private:
    vector<Person> group[4]; // vector容器数组 分别存放一到四组队员信息
};

