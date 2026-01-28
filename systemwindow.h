// systemwindow.h头文件
// 功能说明：系统的窗口类。在对应源文件下将实现全部的功能

#ifndef SYSTEMWINDOW_H
#define SYSTEMWINDOW_H

#include <QMainWindow>
#include "dataFunction.h"
#include "qabstractbutton.h"
#include <QFutureWatcher>
#include <QProgressDialog>
#include "scheduleHistory.h"
#include "historyDialog.h"


QT_BEGIN_NAMESPACE
namespace Ui {
class SystemWindow;
}
QT_END_NAMESPACE
class SystemWindow : public QMainWindow
{
    Q_OBJECT
public:
    SystemWindow(QWidget *parent = nullptr);
    ~SystemWindow();
protected:
    void closeEvent(QCloseEvent *event) override; // 重写 closeEvent 函数，自定义窗口关闭事件
private slots:
    // 值周管理界面槽函数
    // 表格管理
    void onTabulateButtonClicked(); // 排表按钮点击事件
    void onHistoryButtonClicked(); // 查看历史记录按钮点击事件
    void onResetButtonClicked(); // 重置队员执勤总次数按钮点击事件
    void onExportButtonClicked(); // 导出表格按钮点击事件
    void onImportTimeFromTaskButtonClicked(); // 导入空闲时间按钮点击事件（表格管理界面）

    // 制表警告
    void handleSchedulingWarning(const QString& warningMessage); // 排表过程中发送警告信息的对应处理函数

    // 队员管理界面槽函数
    // 组员管理工具栏
    void onGroupAddButtonClicked(int groupIndex); // 添加组员按钮点击事件
    void onGroupDeleteButtonClicked(int groupIndex); // 删除组员按钮点击事件
    void clearMemberInfoDisplay(); // 清空信息显示区
    void onGroupIsWorkRadioButtonClicked(int groupIndex); // 全组是否执勤按钮点击事件
    void onListViewItemClicked(const QModelIndex &index, int groupIndex); // 队员标签点击事件
    // 队员基础信息栏
    void onInfoLineEditChanged(); // 队员基础信息文本框修改事件
    void onGroupComboBoxChanged(int newGroupIndex); // 队员组别信息修改事件

    // 队员事件安排栏
    void onAttendanceButtonClicked(QAbstractButton *button); // 事件安排表中执勤按钮点击事件
    void onAllSelectButtonClicked(); // 全选按钮点击事件
    void onImportTimeButtonClicked(); // 导入空闲时间按钮点击事件
    void onSetAllAvailableButtonClicked(); // 全部可用按钮点击事件
    void onSetAllUnavailableButtonClicked(); // 全部不可用按钮点击事件

private:
    Ui::SystemWindow *ui; // ui界面指针
    SchedulingManager *manager; // 国旗班制表管理器指针
    Flag_group flagGroup; // 国旗班成员容器变量
    Person* currentSelectedPerson = nullptr; // 保存当前用户选中的队员标签指针
    bool isShowingInfo = false; // 新增标志位，用于区分展示信息造成的文本框信息修改和用户主动填写造成的信息修改
    QString warningMessages; // 新增成员变量，用于保存警告信息
    QString filename = "./data/data.txt"; // 保存队员信息的文件名
    bool dataSaved = false; // 标记当前数据是否已保存到文件
    bool hasUnsavedChanges = false; // 标记是否存在未保存的修改
    bool discardWithoutSave = false; // 标记用户是否选择“不保存直接退出”


    QProgressDialog* exportProgress = nullptr; // 进度对话框
    QFutureWatcher<void> exportWatcher;

    ScheduleHistoryManager historyManager; // 历史记录管理器
    QString finalText_excel; // 全局变量，用于导出表格时输出统计的表格信息
    
    // 历史记录相关函数
    void restoreFromHistory(int historyIndex); // 从历史记录恢复状态


    // 值周管理操作函数
    void updateTableWidget(const SchedulingManager& manager); // 制表操作，点击制表按钮后的辅助函数
    void updateTextEdit(const SchedulingManager& manager); // 制表结果在文本域中更新，点击制表按钮后的辅助函数
    void processStep(const QString& stepName, QProgressDialog* progress); // 进度对话框辅助显示函数
    // 队员管理操作函数
    void updateListView(int groupIndex); // 更新队员标签界面
    Person* getSelectedPerson(int groupIndex, const QModelIndex &index); // 捕捉被选中的标签是哪个队员，队员标签点击后的辅助函数
    void showMemberInfo(const Person &person); // 根据选中的队员向UI中展示队员基础信息
    void updatePersonInfo(const Person &person); // 从UI中获取更新后的信息，修改flag_group中队员信息，仅更新基础信息部分，执勤安排不调整（根据程序实际设计，队员组别信息修改不在该函数进行）。
    void updateAttendanceButtons(const Person &person); // 根据队员的time数组调整按钮显示的状态
    bool saveDataToFile(); // 保存数据到文件
    void onApplicationAboutToQuit(); // 应用程序即将退出时的处理
    void markDataChanged(); // 标记数据已被修改
};
#endif // SYSTEMWINDOW_H
