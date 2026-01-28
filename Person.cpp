#include "Person.h"

string Person::getName() const
{
    return name;
}

void Person::setName(const string &newName)
{
    name = newName;
}

bool Person::getGender() const
{
    return gender;
}

void Person::setGender(bool newGender)
{
    gender = newGender;
}

int Person::getGroup() const
{
    return group;
}

void Person::setGroup(int newGroup)
{
    group = newGroup;
}

bool Person::getTime(int row, int column) const
{
    // 获取time数组某一成员的值。调用的参数采用正常思维，row行、column列，最小值为1。
    return time[row-1][column-1];
}

void Person::setTime(bool (newTime[4][5]))
{
    // 设置time数组全部的值。用newTime替换原本的time
    for (int i = 0; i < 4; ++i) {
        for (int g = 0; g < 5; ++g) {
            time[i][g] = newTime[i][g];
        }
    }
}
void Person::setTime(int row, int column, bool value)
{
    // 设置time数组某一成员的值。调用的参数采用正常思维，row行、column列，最小值为1。
    if (row >= 1 && row <= 4 && column >= 1 && column <= 5) {
        time[row - 1][column - 1] = value;
    }

}

int Person::getTimes() const
{
    return times;
}

void Person::setTimes(int newTimes)
{
    times = newTimes;
}

int Person::getAll_times() const
{
    return all_times;
}

void Person::setAll_times(int newAll_times)
{
    all_times = newAll_times;
}

int Person::getNJHAllTimes() const
{
    return njh_all_times;
}

void Person::setNJHAllTimes(int value)
{
    njh_all_times = value;
}

int Person::getDXYAllTimes() const
{
    return dxy_all_times;
}

void Person::setDXYAllTimes(int value)
{
    dxy_all_times = value;
}

string Person::getPhone_number() const
{
    return phone_number;
}

void Person::setPhone_number(const string &newPhone_number)
{
    phone_number = newPhone_number;
}

string Person::getNative_place() const
{
    return native_place;
}

void Person::setNative_place(const string &newNative_place)
{
    native_place = newNative_place;
}

string Person::getNative() const
{
    return native;
}

void Person::setNative(const string &newNative)
{
    native = newNative;
}

string Person::getDorm() const
{
    return dorm;
}

void Person::setDorm(const string &newDorm)
{
    dorm = newDorm;
}

string Person::getSchool() const
{
    return school;
}

void Person::setSchool(const string &newSchool)
{
    school = newSchool;
}

string Person::getClassname() const
{
    return classname;
}

void Person::setClassname(const string &newClassname)
{
    classname = newClassname;
}

bool Person::getIsWork() const
{
    return isWork;
}

void Person::setIsWork(bool newIsWork)
{
    isWork = newIsWork;
}

string Person::getBirthday() const
{
    return birthday;
}

void Person::setBirthday(const string &newBirthday)
{
    birthday = newBirthday;
}

int Person::getGrade() const
{
    return grade;
}

void Person::setGrade(int newGrade)
{
    grade = newGrade;
}
// 无参构造函数
Person::Person() : name(""), gender(false), group(0), grade(0), phone_number(""), native_place(""),
    native(""), dorm(""), school(""), classname(""), birthday(""), isWork(true),
    times(0), all_times(0), njh_all_times(0), dxy_all_times(0) {
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 5; ++j) {
            time[i][j] = false;
        }
    }
}
// 全参构造
Person::Person(const string &name, bool gender, int group, int grade, const string &phone_number, const string &native_place, const string &native, const string &dorm, const string &school, const string &classname, const string &birthday, bool isWork, bool (&time)[4][5], int times, int all_times, int njh_all_times, int dxy_all_times) : name(name),
    gender(gender),
    group(group),
    grade(grade),
    phone_number(phone_number),
    native_place(native_place),
    native(native),
    dorm(dorm),
    school(school),
    classname(classname),
    birthday(birthday),
    isWork(isWork),
    times(times),
    all_times(all_times),
    njh_all_times(njh_all_times),
    dxy_all_times(dxy_all_times)
{
    for (int i = 0; i < 4; ++i) {
        for (int g = 0; g < 5; ++g) {
            this->time[i][g] = time[i][g];
        }
    }
}
