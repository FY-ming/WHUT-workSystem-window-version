// Person.h头文件
// 功能说明：设计队员类Person，用于存放单个队员的基础信息、可工作时间、执勤次数等

#pragma once
#include <string>
using std::string;


class Person
{
public:
    // 构造函数
    Person();
    Person(const string &name, bool gender, int group, int grade,
           const string &phone_number, const string &native_place,
           const string &native, const string &dorm, const string &school,
           const string &classname, const string &birthday, bool isWork, bool (&time)[4][5],
           int times, int all_times, int njh_all_times = 0, int dxy_all_times = 0);
    
    // 重载赋值运算符
    // 对成员变量全部赋新值
    Person& operator=(const Person& other) {
        if (this != &other) {
            name = other.name; // 队员姓名
            gender = other.gender; // 性别（0：男，1：女）
            group = other.group; // 所属组别
            grade = other.grade; // 所属年级
            phone_number = other.phone_number; // 电话号码
            native_place = other.native_place; // 籍贯
            native = other.native; // 民族
            dorm = other.dorm; // 寝室号
            school = other.school; // 学院
            classname = other.classname; // 专业班级
            birthday = other.birthday; // 生日信息
            isWork = other.isWork; // 是否参加执勤标记，用于勾选整组执勤时调用
            for (int i = 0; i < 4; ++i) {
                for (int j = 0; j < 5; ++j) {
                    time[i][j] = other.time[i][j]; // 队员执勤时间安排，对应20个任务时间点是否有时间。一周升降旗十次任务，一次任务两个校区：10*2=20。
                }
            }
            times = other.times; // 一次排班执勤次数，用于记录一周执勤该队员的执勤次数
            all_times = other.all_times; // 学期总执勤次数，用于采用总次数排班规则时使用
            njh_all_times = other.njh_all_times; // 南鉴湖累计执勤次数
            dxy_all_times = other.dxy_all_times; // 东西院累计执勤次数
        }
        return *this;
    }
    // 重载 == 运算符，通过姓名判定是否为同一人
    bool operator==(const Person& other) const {
        return name == other.name;
    }
    // 重载 != 运算符, ==运算符取反
    bool operator!=(const Person& other) const {
        return !(*this == other);
    }

    // get/set函数声明
    // 姓名
    string getName() const;
    void setName(const string &newName);
    // 性别
    bool getGender() const;
    void setGender(bool newGender);
    // 所属组别
    int getGroup() const;
    void setGroup(int newGroup);
    // 所属年级
    int getGrade() const;
    void setGrade(int newGrade);
    // 可执勤时间数组
    bool getTime(int row, int column) const;
    void setTime(bool newtime[4][5]);// 设置time数组全部的值。
    void setTime(int row, int column, bool value);// 设置time数组某一成员的值。调用的参数采用正常思维，row行、column列，最小值为1。
    // 一周执勤次数
    int getTimes() const;
    void setTimes(int newTimes);
    // 学期执勤次数
    int getAll_times() const;
    void setAll_times(int newAll_times);
    // 南鉴湖累计执勤次数
    int getNJHAllTimes() const;
    void setNJHAllTimes(int value);
    // 东西院累计执勤次数
    int getDXYAllTimes() const;
    void setDXYAllTimes(int value);
    // 电话
    string getPhone_number() const;
    void setPhone_number(const string &newPhone_number);
    // 籍贯
    string getNative_place() const;
    void setNative_place(const string &newNative_place);
    // 民族
    string getNative() const;
    void setNative(const string &newNative);
    // 寝室
    string getDorm() const;
    void setDorm(const string &newDorm);
    // 学院
    string getSchool() const;
    void setSchool(const string &newSchool);
    // 专业班级
    string getClassname() const;
    void setClassname(const string &newClassname);
    // 是否参加执勤标记
    bool getIsWork() const;
    void setIsWork(bool newIsWork);
    // 生日信息
    string getBirthday() const;
    void setBirthday(const string &newBirthday);



private:
    // 类成员变量
    // 队员基本信息
    string name; // 队员姓名
    bool gender; // 性别（0：男，1：女）
    int group; // 所属组别（1~4）
    int grade; // 所属年级（1~4）
    string phone_number; // 电话号码
    string native_place; // 籍贯
    string native; // 民族
    string dorm; // 寝室号
    string school; // 学院
    string classname; // 专业班级
    string birthday; // 生日信息


    // 队员执勤所需信息
    bool isWork; // 是否参加执勤标记，用于勾选整组执勤时调用
    bool time[4][5]; // 队员执勤时间安排，对应20个任务时间点是否有时间。一周升降旗十次任务，一次任务两个校区：10*2=20。
    int times; // 一次排班执勤次数，用于记录一周执勤该队员的执勤次数
    int all_times; // 学期总执勤次数，用于采用总次数排班规则时使用
    int njh_all_times; // 南鉴湖累计执勤次数
    int dxy_all_times; // 东西院累计执勤次数
};
