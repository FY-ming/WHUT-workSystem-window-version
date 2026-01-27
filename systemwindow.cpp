// systemwindow.cpp源文件
// 功能说明：构建系统窗口，实现系统中不同组件的具体功能


#include "ui_systemwindow.h"
#include <QMessageBox>
#include <QStringListModel>
#include <QCloseEvent>
#include <QFileDialog>
#include <QAxObject>
#include <QProgressDialog>
#include <QTextStream>
#include <QApplication>
#include <QFileInfo>
#include <QDir>
#include "systemwindow.h"
#include "fileFunction.h"
#include "dataFunction.h"

Flag_group backupGroup; // 全局变量,用于撤销操作
QString finalText_excel; // 全局变量，用于导出表格时输出统计的表格信息

SystemWindow::SystemWindow(QWidget *parent)
    : QMainWindow(parent) // 窗口
    , ui(new Ui::SystemWindow) // ui界面指针
    , manager(nullptr) // 国旗班制表管理器指针
    , flagGroup() // 国旗班成员容器变量
    , currentSelectedPerson(nullptr) // 保存当前用户选中的队员标签指针
    , isShowingInfo(false) // 标志位，用于区分展示信息和用户主动修改
{
    ui->setupUi(this);
    // 将“使用说明”界面的 QTextEdit 文本框设置为只读模式
    ui->instructionText->setReadOnly(true);

    // 在窗口启动时读取文件
    FlagGroupFileManager::loadFromFile(flagGroup, filename);

    // 执勤管理界面
    // 连接按钮和复选框的信号与槽
    connect(ui->tabulateButton, &QPushButton::clicked, this, &SystemWindow::onTabulateButtonClicked); // 排表按钮点击事件
    connect(ui->clearButton, &QPushButton::clicked, this, &SystemWindow::onClearButtonClicked); // 撤销操作按钮点击事件
    connect(ui->alterButton, &QPushButton::clicked, this, &SystemWindow::onResetButtonClicked); // 重置队员执勤总次数按钮点击事件
    connect(ui->deriveButton, &QPushButton::clicked, this, &SystemWindow::onExportButtonClicked); // 导出表格按钮点击事件
    connect(ui->importTimeFromTask_pushButton, &QPushButton::clicked, this, &SystemWindow::onImportTimeFromTaskButtonClicked); // 导入空闲时间按钮点击事件
    // 导出表格、撤销操作在初始时取消交互
    ui->deriveButton->setEnabled(false);
    ui->clearButton->setEnabled(false);
    // 进度对话框指针
    exportProgress = nullptr;
    // 以常规模式为默认排表规则
    ui->normal_mode_radioButton->setChecked(true);
    // 因自定义模式未实现，取消其交互
    ui->custom_mode_radioButton->setEnabled(false);

    // 队员管理界面
    // 更新四个组的队员标签界面
    for (int groupIndex = 1; groupIndex <= 4; ++groupIndex) {
        updateListView(groupIndex);
    }
    // 设置全组执勤按钮的选中状态，默认为被选中
    ui->group1_iswork_radioButton->setChecked(true);
    ui->group2_iswork_radioButton->setChecked(true);
    ui->group3_iswork_radioButton->setChecked(true);
    ui->group4_iswork_radioButton->setChecked(true);
    // 将 FlagGroup 中所有队员的 iswork 信息全部调成 true，对应全组执勤按钮的默认选定状态
    for (int groupIndex = 1; groupIndex <= 4; ++groupIndex) {
        auto& members = flagGroup.getGroupMembers(groupIndex); // 返回对应组的队员列表
        for (auto& member : members) {
            member.setIsWork(true); // 修改iswork信息
        }
    }
    // 设置 availableTime_groupBox 中除"全选"、"导入空闲时间"、"全部可用"、"全部不可用"按钮外的按钮为 checkable
    QList<QAbstractButton*> buttons = ui->availableTime_groupBox->findChildren<QAbstractButton*>();
    for (QAbstractButton* button : buttons) {
        QString buttonText = button->text();
        if (!buttonText.contains("全选") && 
            buttonText != "导入空闲时间" && 
            buttonText != "全部可用" && 
            buttonText != "全部不可用") {
            button->setCheckable(true);
        }
    }
    // 连接“全选”按钮的点击事件
    QList<QAbstractButton*> allSelectButtons = ui->availableTime_groupBox->findChildren<QAbstractButton*>();
    for (QAbstractButton* button : allSelectButtons) {
        if (button->text() == "全选") {
            connect(button, &QAbstractButton::clicked, this, &SystemWindow::onAllSelectButtonClicked);
        }
    }
    // 连接一组的信号与槽
    connect(ui->group1_add_pushButton, &QPushButton::clicked, this, [this]() { onGroupAddButtonClicked(1); });
    connect(ui->group1_delete_pushButton, &QPushButton::clicked, this, [this]() { onGroupDeleteButtonClicked(1); });
    connect(ui->group1_iswork_radioButton, &QRadioButton::clicked, this, [this]() { onGroupIsWorkRadioButtonClicked(1); });
    connect(ui->group1_info_listView, &QListView::clicked, this, [this](const QModelIndex &index) { onListViewItemClicked(index, 1); });
    // 连接二组的信号与槽
    connect(ui->group2_add_pushButton, &QPushButton::clicked, this, [this]() { onGroupAddButtonClicked(2); });
    connect(ui->group2_delete_pushButton, &QPushButton::clicked, this, [this]() { onGroupDeleteButtonClicked(2); });
    connect(ui->group2_iswork_radioButton, &QRadioButton::clicked, this, [this]() { onGroupIsWorkRadioButtonClicked(2); });
    connect(ui->group2_info_listView, &QListView::clicked, this, [this](const QModelIndex &index) { onListViewItemClicked(index, 2); });
    // 连接三组的信号与槽
    connect(ui->group3_add_pushButton, &QPushButton::clicked, this, [this]() { onGroupAddButtonClicked(3); });
    connect(ui->group3_delete_pushButton, &QPushButton::clicked, this, [this]() { onGroupDeleteButtonClicked(3); });
    connect(ui->group3_iswork_radioButton, &QRadioButton::clicked, this, [this]() { onGroupIsWorkRadioButtonClicked(3); });
    connect(ui->group3_info_listView, &QListView::clicked, this, [this](const QModelIndex &index) { onListViewItemClicked(index, 3); });
    // 连接四组的信号与槽
    connect(ui->group4_add_pushButton, &QPushButton::clicked, this, [this]() { onGroupAddButtonClicked(4); });
    connect(ui->group4_delete_pushButton, &QPushButton::clicked, this, [this]() { onGroupDeleteButtonClicked(4); });
    connect(ui->group4_iswork_radioButton, &QRadioButton::clicked, this, [this]() { onGroupIsWorkRadioButtonClicked(4); });
    connect(ui->group4_info_listView, &QListView::clicked, this, [this](const QModelIndex &index) { onListViewItemClicked(index, 4); });

    // 基础信息栏内容
    // 连接 group_combobox 的 currentIndexChanged 信号，组别修改事件
    connect(ui->group_combobox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &SystemWindow::onGroupComboBoxChanged);
    // 连接队员信息输入框的 editingFinished 信号到编辑结束槽函数，但不包括 group_combobox
    connect(ui->name_lineEdit, &QLineEdit::editingFinished, this, &SystemWindow::onInfoLineEditChanged);
    connect(ui->phone_lineEdit, &QLineEdit::editingFinished, this, &SystemWindow::onInfoLineEditChanged);
    connect(ui->nativePlace_lineEdit, &QLineEdit::editingFinished, this, &SystemWindow::onInfoLineEditChanged);
    connect(ui->school_lineEdit, &QLineEdit::editingFinished, this, &SystemWindow::onInfoLineEditChanged);
    connect(ui->native_lineEdit, &QLineEdit::editingFinished, this, &SystemWindow::onInfoLineEditChanged);
    connect(ui->dorm_lineEdit, &QLineEdit::editingFinished, this, &SystemWindow::onInfoLineEditChanged);
    connect(ui->class_lineEdit, &QLineEdit::editingFinished, this, &SystemWindow::onInfoLineEditChanged);
    connect(ui->birthday_lineEdit, &QLineEdit::editingFinished, this, &SystemWindow::onInfoLineEditChanged);
    connect(ui->gender_combobox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &SystemWindow::onInfoLineEditChanged);
    connect(ui->grade_comboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &SystemWindow::onInfoLineEditChanged);
    connect(ui->total_times_spinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &SystemWindow::onInfoLineEditChanged);

    // 连接出勤安排按钮的信号与槽
    connect(ui->monday_up_NJH_pushButton, &QPushButton::clicked, this, [this](bool) {onAttendanceButtonClicked(ui->monday_up_NJH_pushButton);});
    connect(ui->monday_up_DXY_pushButton, &QPushButton::clicked, this, [this](bool) {onAttendanceButtonClicked(ui->monday_up_DXY_pushButton);});
    connect(ui->tuesday_up_NJH_pushButton, &QPushButton::clicked, this, [this](bool) {onAttendanceButtonClicked(ui->tuesday_up_NJH_pushButton);});
    connect(ui->tuesday_up_DXY_pushButton, &QPushButton::clicked, this, [this](bool) {onAttendanceButtonClicked(ui->tuesday_up_DXY_pushButton);});
    connect(ui->wednesday_up_NJH_pushButton, &QPushButton::clicked, this, [this](bool) {onAttendanceButtonClicked(ui->wednesday_up_NJH_pushButton);});
    connect(ui->wednesday_up_DXY_pushButton, &QPushButton::clicked, this, [this](bool) {onAttendanceButtonClicked(ui->wednesday_up_DXY_pushButton);});
    connect(ui->thursday_up_NJH_pushButton, &QPushButton::clicked, this, [this](bool) {onAttendanceButtonClicked(ui->thursday_up_NJH_pushButton);});
    connect(ui->thursday_up_DXY_pushButton, &QPushButton::clicked, this, [this](bool) {onAttendanceButtonClicked(ui->thursday_up_DXY_pushButton);});
    connect(ui->friday_up_NJH_pushButton, &QPushButton::clicked, this, [this](bool) {onAttendanceButtonClicked(ui->friday_up_NJH_pushButton);});
    connect(ui->friday_up_DXY_pushButton, &QPushButton::clicked, this, [this](bool) {onAttendanceButtonClicked(ui->friday_up_DXY_pushButton);});
    connect(ui->monday_down_NJH_pushButton, &QPushButton::clicked, this, [this](bool) {onAttendanceButtonClicked(ui->monday_down_NJH_pushButton);});
    connect(ui->monday_down_DXY_pushButton, &QPushButton::clicked, this, [this](bool) {onAttendanceButtonClicked(ui->monday_down_DXY_pushButton);});
    connect(ui->tuesday_down_NJH_pushButton, &QPushButton::clicked, this, [this](bool) {onAttendanceButtonClicked(ui->tuesday_down_NJH_pushButton);});
    connect(ui->tuesday_down_DXY_pushButton, &QPushButton::clicked, this, [this](bool) {onAttendanceButtonClicked(ui->tuesday_down_DXY_pushButton);});
    connect(ui->wednesday_down_NJH_pushButton, &QPushButton::clicked, this, [this](bool) {onAttendanceButtonClicked(ui->wednesday_down_NJH_pushButton);});
    connect(ui->wednesday_down_DXY_pushButton, &QPushButton::clicked, this, [this](bool) {onAttendanceButtonClicked(ui->wednesday_down_DXY_pushButton);});
    connect(ui->thursday_down_NJH_pushButton, &QPushButton::clicked, this, [this](bool) {onAttendanceButtonClicked(ui->thursday_down_NJH_pushButton);});
    connect(ui->thursday_down_DXY_pushButton, &QPushButton::clicked, this, [this](bool) {onAttendanceButtonClicked(ui->thursday_down_DXY_pushButton);});
    connect(ui->friday_down_NJH_pushButton, &QPushButton::clicked, this, [this](bool) {onAttendanceButtonClicked(ui->friday_down_NJH_pushButton);});
    connect(ui->friday_down_DXY_pushButton, &QPushButton::clicked, this, [this](bool) {onAttendanceButtonClicked(ui->friday_down_DXY_pushButton);});
    // 连接批量操作按钮的信号与槽
    connect(ui->importTime_pushButton, &QPushButton::clicked, this, &SystemWindow::onImportTimeButtonClicked);
    connect(ui->setAllAvailable_pushButton, &QPushButton::clicked, this, &SystemWindow::onSetAllAvailableButtonClicked);
    connect(ui->setAllUnavailable_pushButton, &QPushButton::clicked, this, &SystemWindow::onSetAllUnavailableButtonClicked);
}
SystemWindow::~SystemWindow()
{
    delete exportProgress;
    delete ui;
}
//关闭窗口事件
void SystemWindow::closeEvent(QCloseEvent *event)
{
    // 弹出提示窗口
    QMessageBox::StandardButton reply = QMessageBox::question(this, "关闭系统", "是否关闭系统？", QMessageBox::Yes | QMessageBox::No);
    if (reply == QMessageBox::Yes) {
        // 保存文件
        FlagGroupFileManager::saveToFile(flagGroup, filename);
        // 接受关闭事件
        event->accept();
    } else {
        // 忽略关闭事件，取消关闭行为
        event->ignore();
    }
}

//值周管理界面函数实现
void SystemWindow::onTabulateButtonClicked() {

    // 制表按钮
    if (!manager) {
        backupGroup = flagGroup; // 保存当前状态
        manager = new SchedulingManager(flagGroup);
        connect(manager, &SchedulingManager::schedulingWarning, this, &SystemWindow::handleSchedulingWarning);  // 连接警告信号与发送警告信息的槽函数
        connect(manager, &SchedulingManager::schedulingFinished, this, [this]() {
            ui->tabulateButton->setEnabled(false);
            ui->deriveButton->setEnabled(true);
            ui->clearButton->setEnabled(true);
            updateTableWidget(*manager); // 制表操作
            updateTextEdit(*manager); // 更新制表结果文本域
            delete manager;
            manager = nullptr;
        });
    }

    // 在排表前直接检查单选按钮状态并设置模式
    if (ui->normal_mode_radioButton->isChecked()) {
        manager->setScheduleMode(SchedulingManager::ScheduleMode::Normal);
    } else if (ui->supervisory_mode_radioButton->isChecked()) {
        manager->setScheduleMode(SchedulingManager::ScheduleMode::Supervisory);
    } else if (ui->DXY_only_twice_mode_radioButton->isChecked()) {
        manager->setScheduleMode(SchedulingManager::ScheduleMode::DXYMondayFriday);
    } else if (ui->custom_mode_radioButton->isChecked()) {
        manager->setScheduleMode(SchedulingManager::ScheduleMode::Custom);
    } else {
        // 如果没有选中任何模式，设置一个默认模式
        manager->setScheduleMode(SchedulingManager::ScheduleMode::Normal);
    }
    // 检查可用成员数量，不足则直接返回
    if (manager->getAvailableMembers().size() < 6) {
        QMessageBox::warning(nullptr,"排表错误警告","现在国旗班6个队员都凑不出来了吗:(");
    } else {
        manager->schedule();
    }
}
void SystemWindow::updateTableWidget(const SchedulingManager& manager) {
    //制表操作，点击制表按钮后的辅助函数
    const auto& scheduleTable = manager.getScheduleTable();
    // 从周一上午开始，依次处理表格每个时间槽（周一上午、周一下午、周二上午、周二下午…… 周五下午）
    for (int slot = 0; slot < 10; ++slot) {
        int day = slot / 2; // 0~4，分别对应周一至周五
        int halfDay = slot % 2; // 时段，0~1，分别对应升旗与降旗
        for (int location = 0; location < 2; ++location) {
            // 对于每个时间槽，依次处理两个地点。
            int row = halfDay * 2 + location;// 表格对应的行数，0~3，对应表格单元项的第一到第四行（即不包括表头）
            QString cellText;
            for (int position = 0; position < 3; ++position) {
                // 对于每个地点，检查并添加 3 个人员位置的人员姓名。
                if (scheduleTable[slot][location][position]) {
                    cellText += QString::fromStdString(scheduleTable[slot][location][position]->getName()) + " ";
                }
            }
            ui->worksheet->setItem(row, day, new QTableWidgetItem(cellText.trimmed()));
        }
    }

    // 处理完表格后的表格和窗口大小调整功能

    // 表格大小处理
    // 调整表格列宽以适应内容
    ui->worksheet->resizeColumnsToContents();
    // 调整表格行高以适应内容
    ui->worksheet->resizeRowsToContents();

    // 窗口大小处理
    //
    // 窗口宽度计算
    // 计算表格所需的总宽度
    int totalTableWidth = 0;
    QHeaderView* horizontalHeader = ui->worksheet->horizontalHeader();
    for (int col = 0; col < ui->worksheet->columnCount(); ++col) {
        totalTableWidth += horizontalHeader->sectionSize(col);
    }
    // 加上垂直表头的宽度
    totalTableWidth += ui->worksheet->verticalHeader()->width();
    // 计算表格所需的总高度
    int totalTableHeight = 0;
    QHeaderView* verticalHeader = ui->worksheet->verticalHeader();
    for (int row = 0; row < ui->worksheet->rowCount(); ++row) {
        totalTableHeight += verticalHeader->sectionSize(row);
    }
    //
    // 窗口高度计算
    // 加上水平表头的高度
    totalTableHeight += ui->worksheet->horizontalHeader()->height();
    // 获取 task_toolBox 的宽度
    QWidget* taskToolBox = ui->task_all_splitter->widget(0);
    int taskToolBoxWidth = taskToolBox->width();


    // 获取 timesResult 的高度
    QWidget* timesResult = ui->task_worksheet_splitter->widget(1);
    int timesResultHeight = timesResult->height();
    // 获取窗口当前的布局边距
    QMargins margins = layout()->contentsMargins();
    int marginLeft = margins.left();
    int marginRight = margins.right();
    int marginTop = margins.top();
    int marginBottom = margins.bottom();
    int extraWidth = 80;//补足，人工修改的宽度
    int extraHeight = 10;//补足，人工修改的高度
    // 计算新的宽度和高度
    int newWindowWidth = totalTableWidth + taskToolBoxWidth + marginLeft + marginRight + extraWidth;
    int newWindowHeight = totalTableHeight + timesResultHeight + marginTop + marginBottom + extraHeight;

    // 获取当前窗口的大小
    QSize currentWindowSize = this->size();
    int currentWidth = currentWindowSize.width();
    int currentHeight = currentWindowSize.height();
    // 只放大不缩小
    if (newWindowWidth > currentWidth || newWindowHeight > currentHeight) {
        newWindowWidth = qMax(newWindowWidth, currentWidth);
        newWindowHeight = qMax(newWindowHeight, currentHeight);
        // 调整窗口大小
        resize(newWindowWidth, newWindowHeight);
    }

}

void SystemWindow::handleSchedulingWarning(const QString& warningMessage)
{
    // 排表过程出现无法选出合适人选时的情况，保存警告信息
    warningMessages += warningMessage + "\n";
}

void SystemWindow::updateTextEdit(const SchedulingManager& manager) {
    // 制表结果文本域更新
    QString resultText;
    const auto& availableMembers = manager.getAvailableMembers();
    for (const auto& member : availableMembers) {
        resultText += QString::fromStdString(member->getName()) + " 的工作次数: " + QString::number(member->getTimes()) +
                      " 总工作次数: " + QString::number(member->getAll_times()) + "\n";
    }
    // 拼接警告信息和排班结果文本
    QString finalText = warningMessages + resultText;
    finalText_excel = finalText;
    // 设置最终文本到文本编辑框
    ui->timesResult->setPlainText(finalText);
    // 清空警告信息，以便下次排表使用
    warningMessages.clear();
}
void SystemWindow::onClearButtonClicked() {
    // 撤销操作按钮
    if (backupGroup.isEmpty()) {
        QMessageBox::information(this, "无操作可撤销", "当前无排表记录可撤销");
        return;
    }
    // 恢复备份数据
    flagGroup = backupGroup;

    // 刷新表格和统计信息
    ui->worksheet->clearContents();
    ui->timesResult->clear();
    QMessageBox::information(this, "撤销成功", "已恢复至上一次排表前状态");
    ui->deriveButton->setEnabled(false);
    ui->clearButton->setEnabled(false);
    ui->tabulateButton->setEnabled(true);
}
void SystemWindow::onResetButtonClicked() {
    //重置队员执勤次数按钮
    for (int i = 1; i < 5; ++i) {
        auto& allMembers = flagGroup.getGroupMembers(i);
        for (auto& member : allMembers) {
            member.setAll_times(0);
        }
    }
    // 在 QTextEdit 中清空并输出提示语句
    ui->timesResult->clear();
    ui->timesResult->append("所有队员的执勤总次数已成功归零");
}
// 导出表格颜色转换辅助函数
int rgbToBgr(const QColor& color) {
    return (color.blue() << 16) | (color.green() << 8) | color.red();
}
void SystemWindow::onExportButtonClicked()
{
    // 导出表格按钮点击事件
    // 获取保存文件路径
    QString filePath = QFileDialog::getSaveFileName(this, "导出表格", "第X周升降旗.xlsx", "Excel 文件 (*.xlsx)");
    // 检查文件路径
    if (filePath.isEmpty()) return;

    // 确保进度对话框被正确初始化和重置
    if (exportProgress == nullptr) {
        exportProgress = new QProgressDialog(this);
        exportProgress->setWindowModality(Qt::ApplicationModal);
        exportProgress->setWindowFlags(exportProgress->windowFlags() | Qt::WindowStaysOnTopHint);
        exportProgress->setCancelButton(nullptr);
        exportProgress->setMinimumDuration(0); // 允许快速显示
    } else {
        // 重置对话框状态
        exportProgress->reset();
        exportProgress->setWindowModality(Qt::ApplicationModal);
        exportProgress->setWindowFlags(exportProgress->windowFlags() | Qt::WindowStaysOnTopHint);
        exportProgress->setCancelButton(nullptr);
        exportProgress->setMinimumDuration(0);
    }

    // 显示初始提示信息
    exportProgress->setWindowTitle("导出进度");
    exportProgress->setLabelText("正在导出表格，请稍候...");
    exportProgress->setRange(0, 0); // 不确定进度模式
    exportProgress->show();

    // 强制更新UI，确保初始提示显示
    QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
    QApplication::processEvents(QEventLoop::ExcludeSocketNotifiers);
    QApplication::processEvents(QEventLoop::WaitForMoreEvents, 100); // 强制显示100ms

    QAxObject *excel = new QAxObject("Excel.Application");
    if (excel) {
        // 设置 Excel 应用程序不可见
        excel->dynamicCall("SetVisible(bool)", false);
        // 获取 Excel 应用程序的工作簿集合
        QAxObject *workbooks = excel->querySubObject("Workbooks");
        // 分段处理：每完成一个主要步骤就处理一次事件
        processStep("创建工作簿", exportProgress); // 自定义函数，见下方
        // 创建一个新的工作簿
        QAxObject *workbook = workbooks->querySubObject("Add");
        if (workbook) {
            // ========== 第一个工作表：统计信息 ==========
            // 获取新工作簿的第一个工作表
            QAxObject *statsSheet = workbook->querySubObject("Worksheets(int)", 1);
            statsSheet->dynamicCall("SetName(const QString&)", "统计信息");
            // 将 finalText 内容逐行写入工作表
            QStringList lines = finalText_excel.split('\n');
            for (int i = 0; i < lines.size(); ++i) {
                QAxObject *cell = statsSheet->querySubObject("Cells(int,int)", i + 1, 1);
                cell->dynamicCall("SetValue(const QVariant&)", lines[i]);
                // 获取字体对象并设置颜色为蓝色
                QAxObject *font = cell->querySubObject("Font");
                if (font) {
                    // RGB 值转换为 BGR（Excel 使用 BGR 格式）
                    int blueColor = rgbToBgr(QColor(Qt::blue));
                    font->dynamicCall("SetColor(int)", blueColor);
                    delete font; // 释放对象，避免内存泄漏
                }
                // 处理事件，让进度对话框有机会更新
                if (i % 10 == 0) { // 每10行处理一次事件
                    QApplication::processEvents();
                    if (exportProgress->wasCanceled()) { // 虽然移除了取消按钮，但仍可检查状态
                        break;
                    }
                }
            }
            // 调整列宽以适应内容
            QAxObject *usedRange = statsSheet->querySubObject("UsedRange");
            usedRange->querySubObject("Columns")->dynamicCall("AutoFit()");

            processStep("写入统计信息", exportProgress);


            // ========== 第二个工作表：排班表 ==========
            QAxObject *worksheets = workbook->querySubObject("Worksheets");
            worksheets->dynamicCall("Add()"); // 添加新工作表
            QAxObject *worksheetExcel = workbook->querySubObject("Worksheets(int)", 1);
            worksheetExcel->dynamicCall("SetName(const QString&)", "排班表");

            processStep("创建排班表", exportProgress);

            // 1. 所有拥有文字的区域（A1~G5矩阵区域）都应该居中对齐
            QAxObject *allRange = worksheetExcel->querySubObject("Range(const QString&)", "A1:G5");
            allRange->setProperty("HorizontalAlignment", -4108); // 直接设置属性
            // 2. 第一行列标题C1~G1区域，文本内容不变，字体格式改为16号黑体，背景填充色改为#FFC000
            QAxObject *headerRange = worksheetExcel->querySubObject("Range(const QString&)", "C1:G1");
            QAxObject *headerFont = headerRange->querySubObject("Font");
            headerFont->dynamicCall("SetName(const QString&)", "黑体");
            headerFont->dynamicCall("SetSize(int)", 16);
            QAxObject *headerInterior = headerRange->querySubObject("Interior");
            QColor Color("#FFC000");
            int BgrColor = rgbToBgr(Color);
            headerInterior->dynamicCall("SetColor(int)", BgrColor);
            // 复制列标题到 Excel
            for (int col = 0; col < ui->worksheet->columnCount(); ++col) {
                QString headerText = ui->worksheet->horizontalHeaderItem(col)->text();
                QAxObject *cell = worksheetExcel->querySubObject("Cells(int,int)", 1, col + 3); // 从第一行第三列开始写列标题
                cell->dynamicCall("SetValue(const QVariant&)", headerText);
            }
            processStep("设置表头格式", exportProgress);
            // 3. 第一列A2和A3区域合并，并输入“升旗”文本，字体格式改为16号黑体，背景填充色改为#FFFF00
            // 两者使用的颜色单位不同，不能直接转换需要rgb to bgr的操作
            QAxObject *riseFlagRange = worksheetExcel->querySubObject("Range(const QString&)", "A2:A3");
            riseFlagRange->dynamicCall("Merge()");
            QAxObject *riseFlagCell = worksheetExcel->querySubObject("Cells(int,int)", 2, 1);
            riseFlagCell->dynamicCall("SetValue(const QVariant&)", "升旗");
            QAxObject *riseFlagFont = riseFlagRange->querySubObject("Font");
            riseFlagFont->dynamicCall("SetName(const QString&)", "黑体");
            riseFlagFont->dynamicCall("SetSize(int)", 16);
            QAxObject *riseFlagInterior = riseFlagRange->querySubObject("Interior");
            Color = QColor("#FFF000");
            BgrColor = rgbToBgr(Color);
            riseFlagInterior->dynamicCall("SetColor(int)", BgrColor);
            processStep("设置升旗区域", exportProgress);
            // 4. 第一列A4和A5区域合并，并输入“降旗”文本，字体格式改为16号黑体，背景填充色改为#FFFF00
            QAxObject *lowerFlagRange = worksheetExcel->querySubObject("Range(const QString&)", "A4:A5");
            lowerFlagRange->dynamicCall("Merge()");
            QAxObject *lowerFlagCell = worksheetExcel->querySubObject("Cells(int,int)", 4, 1);
            lowerFlagCell->dynamicCall("SetValue(const QVariant&)", "降旗");
            QAxObject *lowerFlagFont = lowerFlagRange->querySubObject("Font");
            lowerFlagFont->dynamicCall("SetName(const QString&)", "黑体");
            lowerFlagFont->dynamicCall("SetSize(int)", 16);
            QAxObject *lowerFlagInterior = lowerFlagRange->querySubObject("Interior");
            Color = QColor("#FFF000");
            BgrColor = rgbToBgr(Color);
            lowerFlagInterior->dynamicCall("SetColor(int)", BgrColor);
            processStep("设置降旗区域", exportProgress);
            // 5. 第二列B2~B5区域行标题，文本内容不变，字体格式改为12号等线，背景填充色改为#FFFF00
            QAxObject *rowHeaderRange = worksheetExcel->querySubObject("Range(const QString&)", "B2:B5");
            QAxObject *rowHeaderFont = rowHeaderRange->querySubObject("Font");
            rowHeaderFont->dynamicCall("SetName(const QString&)", "等线");
            rowHeaderFont->dynamicCall("SetSize(int)", 12);
            QAxObject *rowHeaderInterior = rowHeaderRange->querySubObject("Interior");
            Color = QColor("#FFF000");
            BgrColor = rgbToBgr(Color);
            rowHeaderInterior->dynamicCall("SetColor(int)", BgrColor);
            processStep("设置行标题", exportProgress);
            // 复制行标题到 Excel
            for (int row = 0; row < ui->worksheet->rowCount(); ++row) {
                QString rowHeaderText = ui->worksheet->verticalHeaderItem(row)->text();
                QAxObject *cell = worksheetExcel->querySubObject("Cells(int,int)", row + 2, 2); // 从第二行第二列写行标题
                cell->dynamicCall("SetValue(const QVariant&)", rowHeaderText);
            }
            // 复制表格数据到 Excel，从第二行第三列开始
            for (int row = 0; row < ui->worksheet->rowCount(); ++row) {
                for (int col = 0; col < ui->worksheet->columnCount(); ++col) {
                    QTableWidgetItem *item = ui->worksheet->item(row, col);
                    if (item) {
                        QAxObject *cell = worksheetExcel->querySubObject("Cells(int,int)", row + 2, col + 3);
                        cell->dynamicCall("SetValue(const QVariant&)", item->text());
                    }
                }
            }
            processStep("复制表格数据", exportProgress);
            // // 6. A1~G5矩阵区域设置粗外侧框线
            QAxObject *allBorders = allRange->querySubObject("Borders");
            if (allBorders) {
                // 设置边框样式为连续线条
                allBorders->dynamicCall("LineStyle", 1); // xlContinuous
                // 设置边框粗细为粗线
                allBorders->dynamicCall("Weight", 2);    // xlThick
                delete allBorders;
            }
            //C1~G1区域、B2~B5区域、A2和A3的合并区域、A4和A5的合并区域分别设置为所有框线
            QAxObject *headerBorders = headerRange->querySubObject("Borders");
            headerBorders->dynamicCall("LineStyle", 1); // xlContinuous
            QAxObject *rowHeaderBorders = rowHeaderRange->querySubObject("Borders");
            rowHeaderBorders->dynamicCall("LineStyle", 1); // xlContinuous
            QAxObject *riseFlagBorders = riseFlagRange->querySubObject("Borders");
            riseFlagBorders->dynamicCall("LineStyle", 1); // xlContinuous
            QAxObject *lowerFlagBorders = lowerFlagRange->querySubObject("Borders");
            lowerFlagBorders->dynamicCall("LineStyle", 1); // xlContinuous
            processStep("设置表格边框", exportProgress);
            // 7. A列宽110像素，B列宽175像素，C~G列宽350像素，1~5行高全部设置为65像素
            // 两者计量单位不同，需要计算后转换
            QAxObject *columnA = worksheetExcel->querySubObject("Columns(const QString&)", "A");
            columnA->dynamicCall("ColumnWidth", 8);
            QAxObject *columnB = worksheetExcel->querySubObject("Columns(const QString&)", "B");
            columnB->dynamicCall("ColumnWidth", 14);
            QAxObject *columnsCToG = worksheetExcel->querySubObject("Range(const QString&)", "C:G");
            columnsCToG->dynamicCall("ColumnWidth", 28);
            QAxObject *rows1To5 = worksheetExcel->querySubObject("Range(const QString&)", "1:5");
            rows1To5->dynamicCall("RowHeight", 32.5);
            processStep("设置表格格式", exportProgress);
            // 保存并关闭 Excel 文件
            // 将工作簿保存到用户指定的路径
            workbook->dynamicCall("SaveAs(const QString&)", QDir::toNativeSeparators(filePath));
            // 关闭工作簿
            workbook->dynamicCall("Close()");
            processStep("保存工作簿", exportProgress);
        }
        // 退出 Excel 应用程序
        excel->dynamicCall("Quit()");
        // 释放 Excel 应用程序对象的内存
        delete excel;
    }


    // 导出完成后恢复控件状态并提示
    if (exportProgress) {
        exportProgress->hide();
    }
    QMessageBox::information(this, "导出完成", "表格已成功导出至：\n" + filePath, QMessageBox::Ok);

}
// 自定义进度更新函数
void SystemWindow::processStep(const QString& stepName, QProgressDialog* progress) {
    if (progress && progress->isVisible()) {
        progress->setLabelText("正在导出表格 - " + stepName + "...");
        QApplication::processEvents();
    }
}


//队员管理界面函数实现
void SystemWindow::onGroupAddButtonClicked(int groupIndex)
{
    //添加队员按钮点击事件
    bool isChecked = false;
    //判断是哪个组发出的信号，修改新队员的是否值周状态
    switch (groupIndex) {
    case 1: isChecked = ui->group1_iswork_radioButton->isChecked(); break;
    case 2: isChecked = ui->group2_iswork_radioButton->isChecked(); break;
    case 3: isChecked = ui->group3_iswork_radioButton->isChecked(); break;
    case 4: isChecked = ui->group4_iswork_radioButton->isChecked(); break;
    } 
    bool time[4][5];
    //初始化所有执勤时间，默认为全部不可以执勤
    for (int i = 0; i < 4; ++i) {
        for (int g = 0; g < 5; ++g) {
            time[i][g] = false;
        }
    }

    // 生成唯一的默认名字
    std::string defaultNameBase = "未命名队员";
    int counter = 1;
    std::string defaultName = defaultNameBase + std::to_string(counter);

    // 获取所有队员的名字
    std::vector<std::string> existingNames;
    for (int i = 1; i <= 4; ++i) {
        const auto& members = flagGroup.getGroupMembers(i);
        for (const auto& member : members) {
            existingNames.push_back(member.getName());
        }
    }

    // 检查名字是否重复，若重复则递增计数器
    while (std::find(existingNames.begin(), existingNames.end(), defaultName) != existingNames.end()) {
        counter++;
        defaultName = defaultNameBase + std::to_string(counter);
    }
    //初始化新队员的基础信息
    Person person(defaultName, false, groupIndex, 1, "", "", "", "", "", "", "", isChecked, time, 0, 0);
    //向flagGroup中添加新队员
    flagGroup.addPersonToGroup(person, groupIndex);
    //更新对应组的ListView组员标签信息
    updateListView(groupIndex);
}
void SystemWindow::onGroupDeleteButtonClicked(int groupIndex)
{
    //删除队员按钮点击事件
    QListView* listView = nullptr;
    //判断是哪个组发出的信号，以及信号情况
    switch (groupIndex) {
    case 1: listView = ui->group1_info_listView; break;
    case 2: listView = ui->group2_info_listView; break;
    case 3: listView = ui->group3_info_listView; break;
    case 4: listView = ui->group4_info_listView; break;
    }
    //如果出现非法组号，退出
    if (!listView) return;
    QModelIndexList selectedIndexes = listView->selectionModel()->selectedIndexes();
    //不存在被选定的队员
    if (selectedIndexes.isEmpty()) return;
    //删除前的确认弹窗
    QMessageBox::StandardButton reply = QMessageBox::question(this, "确认删除", "是否删除该队员？", QMessageBox::Yes | QMessageBox::No);
    //删除对应队员
    if (reply == QMessageBox::Yes) {
        int row = selectedIndexes.first().row();
        const auto& members = flagGroup.getGroupMembers(groupIndex);
        if (static_cast<std::vector<Person>::size_type>(row) < members.size()) {
            const Person& personToRemove = members[row];
            // 检查删除的是否是当前显示的队员
            if (currentSelectedPerson && *currentSelectedPerson == personToRemove) {
                clearMemberInfoDisplay(); // 清空信息显示区
                currentSelectedPerson = nullptr; // 重置当前选中队员指针
            }
            flagGroup.removePersonFromGroup(personToRemove, groupIndex);// 调用 Flag_group 的删除成员方法
            updateListView(groupIndex);//更新对应组的ListView组员标签信息
        }
    }
}
// 新增的清空信息显示区函数
void SystemWindow::clearMemberInfoDisplay()
{
    ui->name_lineEdit->clear();
    ui->phone_lineEdit->clear();
    ui->nativePlace_lineEdit->clear();
    ui->school_lineEdit->clear();
    ui->native_lineEdit->clear();
    ui->dorm_lineEdit->clear();
    ui->class_lineEdit->clear();
    ui->birthday_lineEdit->clear();
    ui->gender_combobox->setCurrentIndex(0);
    ui->grade_comboBox->setCurrentIndex(0);
    ui->group_combobox->setCurrentIndex(0);
    ui->total_times_spinBox->setValue(0);

    // 清空执勤时间按钮状态
    QList<QAbstractButton*> attendanceButtons = ui->availableTime_groupBox->findChildren<QAbstractButton*>();
    for (QAbstractButton* button : attendanceButtons) {
        if (!button->objectName().endsWith("all_pushButton")) {
            button->setChecked(false);
        }
    }
}
void SystemWindow::onGroupIsWorkRadioButtonClicked(int groupIndex)
{
    //是否值周确认按钮点击事件
    bool isChecked = false;
    //判断是哪个组发出的信号，以及信号情况
    switch (groupIndex) {
    case 1: isChecked = ui->group1_iswork_radioButton->isChecked(); break;
    case 2: isChecked = ui->group2_iswork_radioButton->isChecked(); break;
    case 3: isChecked = ui->group3_iswork_radioButton->isChecked(); break;
    case 4: isChecked = ui->group4_iswork_radioButton->isChecked(); break;
    }
    //获取对应组别所有队员
    const auto& members = flagGroup.getGroupMembers(groupIndex);
    //设置对应组别所有队员isWork属性，选中设为1，取消选中设为0
    for (auto& member : const_cast<std::vector<Person>&>(members)) {
        member.setIsWork(isChecked);
    }
}
void SystemWindow::onListViewItemClicked(const QModelIndex &index, int groupIndex)
{
    //队员标签点击事件
    //显示选中队员的信息
    Person* person = getSelectedPerson(groupIndex, index);//捕捉被点击的队员是谁
    if (person) {
        currentSelectedPerson = person;
        isShowingInfo = true; // 设置标志位为展示信息状态
        showMemberInfo(*person);//显示基础信息
        updateAttendanceButtons(*person);//显示执勤信息
        isShowingInfo = false; // 恢复标志位
    }
}
Person* SystemWindow::getSelectedPerson(int groupIndex, const QModelIndex &index)
{
    //捕捉被选中的标签是哪个队员
    const auto& members = flagGroup.getGroupMembers(groupIndex);
    if (index.isValid() && static_cast<std::vector<Person>::size_type>(index.row()) < members.size()){
        return const_cast<Person*>(&members[index.row()]);
    }
    return nullptr;
}
void SystemWindow::onInfoLineEditChanged()
{
    // 信息修改后更新 Flag_group 中队员的信息,不包括点击队员标签时显示队员信息时造成的修改
    if (currentSelectedPerson && !isShowingInfo) {
        updatePersonInfo(*currentSelectedPerson);
    }
}
void SystemWindow::onGroupComboBoxChanged(int newGroupIndex)
{
    // 独立的修改组别函数
    if (currentSelectedPerson && !isShowingInfo) {
        int oldGroupIndex = currentSelectedPerson->getGroup();//值为1~4
        if((oldGroupIndex - 1) != newGroupIndex)//规避并未修改组别引发多余操作
        {
            newGroupIndex += 1; // combobox 索引从 0 开始，组索引从 1 开始
            currentSelectedPerson->setGroup(newGroupIndex);// 更新队员的组别信息
            bool isChecked = false;
            switch (newGroupIndex) {
            case 1: isChecked = ui->group1_iswork_radioButton->isChecked(); break;
            case 2: isChecked = ui->group2_iswork_radioButton->isChecked(); break;
            case 3: isChecked = ui->group3_iswork_radioButton->isChecked(); break;
            case 4: isChecked = ui->group4_iswork_radioButton->isChecked(); break;
            }
            currentSelectedPerson->setIsWork(isChecked);
            flagGroup.addPersonToGroup(*currentSelectedPerson, newGroupIndex);// 将队员添加到新组中
            updateListView(newGroupIndex);// 更新新组组别信息
            Person* person = flagGroup.findPersonInGroup(*currentSelectedPerson, newGroupIndex);//创建一个新的队员指针跟踪新创建的队员
            flagGroup.removePersonFromGroup(*currentSelectedPerson, oldGroupIndex);// 从旧组中删除队员
            updateListView(oldGroupIndex);// 更新旧组组别信息
            currentSelectedPerson = person;// 更新当前选中的队员指针
        }
    }
}
void SystemWindow::updateListView(int groupIndex)
{
    // 更新队员标签界面
    QListView* listView = nullptr;
    switch (groupIndex) {
    case 1: listView = ui->group1_info_listView; break;
    case 2: listView = ui->group2_info_listView; break;
    case 3: listView = ui->group3_info_listView; break;
    case 4: listView = ui->group4_info_listView; break;
    }
    if (!listView) return;
    const auto& members = flagGroup.getGroupMembers(groupIndex);
    QStringList memberNames;
    for (const auto& member : members) {
        memberNames << QString::fromStdString(member.getName());
    }
    // 使用 QStringListModel 替代 QStandardItemModel
    QStringListModel *model = new QStringListModel();//创建一个 QStringListModel 对象 model，它是 QAbstractItemModel 的子类，专门用于处理字符串列表数据
    model->setStringList(memberNames);//调用 model 的 setStringList 方法，将 memberNames 列表中的数据设置到模型中。
    listView->setModel(model);//调用 listView 的 setModel 方法，将 model 设置为 listView 的数据模型。这样，listView 就会显示 model 中的数据，即指定组别的队员姓名。
    // 关键设置：禁用所有编辑触发，但保留点击功能
    listView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    // 可选：设置选择模式，允许点击选择
    listView->setSelectionMode(QAbstractItemView::SingleSelection);
}
void SystemWindow::showMemberInfo(const Person &person)
{
    // 根据选中的队员向UI中展示队员基础信息
    ui->name_lineEdit->setText(QString::fromStdString(person.getName()));
    ui->phone_lineEdit->setText(QString::fromStdString(person.getPhone_number()));
    ui->nativePlace_lineEdit->setText(QString::fromStdString(person.getNative_place()));
    ui->school_lineEdit->setText(QString::fromStdString(person.getSchool()));
    ui->native_lineEdit->setText(QString::fromStdString(person.getNative()));
    ui->dorm_lineEdit->setText(QString::fromStdString(person.getDorm()));
    ui->class_lineEdit->setText(QString::fromStdString(person.getClassname()));
    ui->birthday_lineEdit->setText(QString::fromStdString(person.getBirthday()));
    ui->gender_combobox->setCurrentText(person.getGender() ? "女" : "男");
    ui->grade_comboBox->setCurrentIndex(person.getGrade() - 1);//grade_combobox默认以0开始，与年级最小为1相违背
    ui->group_combobox->setCurrentIndex(person.getGroup() - 1);//group_combobox默认以0开始，与组名最小为1相违背
    ui->total_times_spinBox->setValue(person.getAll_times());
}
void SystemWindow::updatePersonInfo(const Person &person)
{
    // 仅更新基础信息部分，执勤安排不调整
    // 从 UI 中获取更新后的信息，修改flag_group中队员信息
    QString name = ui->name_lineEdit->text();
    QString phone = ui->phone_lineEdit->text();
    QString nativePlace = ui->nativePlace_lineEdit->text();
    QString school = ui->school_lineEdit->text();
    QString native = ui->native_lineEdit->text();
    QString dorm = ui->dorm_lineEdit->text();
    QString classname = ui->class_lineEdit->text();
    QString birthday = ui->birthday_lineEdit->text();
    bool gender = ui->gender_combobox->currentText() == "女";
    int grade = ui->grade_comboBox->currentIndex() + 1;
    int all_times = ui->total_times_spinBox->value();
    // 创建新的 Person 对象
    bool time[4][5];
    //time 数组保持不变
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 5; ++j) {
            time[i][j] = person.getTime(i + 1, j + 1);
        }
    }
    Person newPerson(name.toStdString(), gender, person.getGroup(), grade, phone.toStdString(),
                     nativePlace.toStdString(), native.toStdString(), dorm.toStdString(),
                     school.toStdString(), classname.toStdString(), birthday.toStdString(), person.getIsWork(),
                     time, person.getTimes(), all_times);
    // 更新 flagGroup 中对应队员的信息
    flagGroup.modifyPersonInGroup(person, newPerson, person.getGroup());
    // 更新对应组的 ListView 显示
    updateListView(person.getGroup());

}
void SystemWindow::updateAttendanceButtons(const Person &person)
{
    // 根据队员的time数组调整按钮显示的状态
    // 周一升旗
    ui->monday_up_NJH_pushButton->setChecked(person.getTime(1, 1));
    ui->monday_up_DXY_pushButton->setChecked(person.getTime(2, 1));
    // 周二升旗
    ui->tuesday_up_NJH_pushButton->setChecked(person.getTime(1, 2));
    ui->tuesday_up_DXY_pushButton->setChecked(person.getTime(2, 2));
    // 周三升旗
    ui->wednesday_up_NJH_pushButton->setChecked(person.getTime(1, 3));
    ui->wednesday_up_DXY_pushButton->setChecked(person.getTime(2, 3));
    // 周四升旗
    ui->thursday_up_NJH_pushButton->setChecked(person.getTime(1, 4));
    ui->thursday_up_DXY_pushButton->setChecked(person.getTime(2, 4));
    // 周五升旗
    ui->friday_up_NJH_pushButton->setChecked(person.getTime(1, 5));
    ui->friday_up_DXY_pushButton->setChecked(person.getTime(2, 5));
    // 周一降旗
    ui->monday_down_NJH_pushButton->setChecked(person.getTime(3, 1));
    ui->monday_down_DXY_pushButton->setChecked(person.getTime(4, 1));
    // 周二降旗
    ui->tuesday_down_NJH_pushButton->setChecked(person.getTime(3, 2));
    ui->tuesday_down_DXY_pushButton->setChecked(person.getTime(4, 2));
    // 周三降旗
    ui->wednesday_down_NJH_pushButton->setChecked(person.getTime(3, 3));
    ui->wednesday_down_DXY_pushButton->setChecked(person.getTime(4, 3));
    // 周四降旗
    ui->thursday_down_NJH_pushButton->setChecked(person.getTime(3, 4));
    ui->thursday_down_DXY_pushButton->setChecked(person.getTime(4, 4));
    // 周五降旗
    ui->friday_down_NJH_pushButton->setChecked(person.getTime(3, 5));
    ui->friday_down_DXY_pushButton->setChecked(person.getTime(4, 5));
}
void SystemWindow::onAttendanceButtonClicked(QAbstractButton *button)
{
    // 执勤按钮点击事件
    // 根据按钮修改time数组信息
    Person* currentPerson = currentSelectedPerson;
    if (currentPerson) {
        // 定义按钮到 (row, column) 的映射,使用映射表来存储按钮和对应的 setTime 方法所需的行、列参数，这样可以避免大量重复的条件判断。
        static QMap<QAbstractButton*, std::pair<int, int>> buttonToTimeMap = {
            {ui->monday_up_NJH_pushButton, {1, 1}},
            {ui->monday_up_DXY_pushButton, {2, 1}},
            {ui->tuesday_up_NJH_pushButton, {1, 2}},
            {ui->tuesday_up_DXY_pushButton, {2, 2}},
            {ui->wednesday_up_NJH_pushButton, {1, 3}},
            {ui->wednesday_up_DXY_pushButton, {2, 3}},
            {ui->thursday_up_NJH_pushButton, {1, 4}},
            {ui->thursday_up_DXY_pushButton, {2, 4}},
            {ui->friday_up_NJH_pushButton, {1, 5}},
            {ui->friday_up_DXY_pushButton, {2, 5}},
            {ui->monday_down_NJH_pushButton, {3, 1}},
            {ui->monday_down_DXY_pushButton, {4, 1}},
            {ui->tuesday_down_NJH_pushButton, {3, 2}},
            {ui->tuesday_down_DXY_pushButton, {4, 2}},
            {ui->wednesday_down_NJH_pushButton, {3, 3}},
            {ui->wednesday_down_DXY_pushButton, {4, 3}},
            {ui->thursday_down_NJH_pushButton, {3, 4}},
            {ui->thursday_down_DXY_pushButton, {4, 4}},
            {ui->friday_down_NJH_pushButton, {3, 5}},
            {ui->friday_down_DXY_pushButton, {4, 5}}
        };
        // 查找按钮对应的 (row, column)
        auto it = buttonToTimeMap.find(button);
        if (it != buttonToTimeMap.end()) {
            int row = it.value().first;
            int column = it.value().second;
            currentPerson->setTime(row, column, button->isChecked());
        }
    }
}
void SystemWindow::onAllSelectButtonClicked()
{
    //全选按钮点击事件
    QAbstractButton* senderButton = qobject_cast<QAbstractButton*>(sender());
    if (!senderButton || !currentSelectedPerson) return;
    // 获取当前点击的“全选”按钮所在的 groupBox
    QGroupBox* parentGroupBox = qobject_cast<QGroupBox*>(senderButton->parent());
    if (!parentGroupBox) return;
    // 获取 groupBox 中的其他两个按钮
    QList<QAbstractButton*> childButtons = parentGroupBox->findChildren<QAbstractButton*>();
    for (QAbstractButton* button : childButtons) {
        if (button != senderButton) {
            button->setChecked(true);
        }
    }
    // 更新对应 time 数组
    static QMap<QAbstractButton*, std::pair<int, int>> buttonToTimeMap = {
        {ui->monday_up_NJH_pushButton, {1, 1}},
        {ui->monday_up_DXY_pushButton, {2, 1}},
        {ui->tuesday_up_NJH_pushButton, {1, 2}},
        {ui->tuesday_up_DXY_pushButton, {2, 2}},
        {ui->wednesday_up_NJH_pushButton, {1, 3}},
        {ui->wednesday_up_DXY_pushButton, {2, 3}},
        {ui->thursday_up_NJH_pushButton, {1, 4}},
        {ui->thursday_up_DXY_pushButton, {2, 4}},
        {ui->friday_up_NJH_pushButton, {1, 5}},
        {ui->friday_up_DXY_pushButton, {2, 5}},
        {ui->monday_down_NJH_pushButton, {3, 1}},
        {ui->monday_down_DXY_pushButton, {4, 1}},
        {ui->tuesday_down_NJH_pushButton, {3, 2}},
        {ui->tuesday_down_DXY_pushButton, {4, 2}},
        {ui->wednesday_down_NJH_pushButton, {3, 3}},
        {ui->wednesday_down_DXY_pushButton, {4, 3}},
        {ui->thursday_down_NJH_pushButton, {3, 4}},
        {ui->thursday_down_DXY_pushButton, {4, 4}},
        {ui->friday_down_NJH_pushButton, {3, 5}},
        {ui->friday_down_DXY_pushButton, {4, 5}}
    };
    for (QAbstractButton* button : childButtons) {
        if (button != senderButton) {
            auto it = buttonToTimeMap.find(button);
            if (it != buttonToTimeMap.end()) {
                int row = it.value().first;
                int column = it.value().second;
                currentSelectedPerson->setTime(row, column, true);
            }
        }
    }
}
// 从CSV或Excel文件导入空闲时间
void SystemWindow::onImportTimeButtonClicked()
{
    QString filePath = QFileDialog::getOpenFileName(this, "导入空闲时间", "", 
        "支持的文件 (*.csv *.xlsx *.xls);;CSV 文件 (*.csv);;Excel 文件 (*.xlsx *.xls)");
    if (filePath.isEmpty()) {
        return;
    }

    int successCount = 0;
    int failCount = 0;

    // 根据文件扩展名判断文件类型
    QString suffix = QFileInfo(filePath).suffix().toLower();
    
    if (suffix == "csv") {
        // 处理CSV文件
        QFile file(filePath);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QMessageBox::warning(this, "错误", "无法打开文件：" + filePath);
            return;
        }

        QTextStream in(&file);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
        in.setCodec("UTF-8");
#endif

        // 跳过表头
        if (!in.atEnd()) {
            in.readLine();
        }

        // 读取数据
        while (!in.atEnd()) {
            QString line = in.readLine();
            if (line.isEmpty()) continue;

            QStringList parts = line.split(",");
            if (parts.size() < 22) { // 姓名+组别+20个时间点
                failCount++;
                continue;
            }

            QString name = parts[0];
            int group = parts[1].toInt();
            if (group < 1 || group > 4) {
                failCount++;
                continue;
            }

            // 查找对应的队员
            bool found = false;
            auto& members = flagGroup.getGroupMembers(group);
            for (auto& member : members) {
                if (QString::fromStdString(member.getName()) == name) {
                    // 读取时间安排
                    bool time[4][5];
                    for (int row = 0; row < 4; ++row) {
                        for (int col = 0; col < 5; ++col) {
                            int index = 2 + row * 5 + col;
                            if (index < parts.size()) {
                                time[row][col] = (parts[index].toInt() == 1);
                            } else {
                                time[row][col] = false;
                            }
                        }
                    }
                    member.setTime(time);
                    found = true;
                    successCount++;
                    break;
                }
            }
            if (!found) {
                failCount++;
            }
        }

        file.close();
    } else if (suffix == "xlsx" || suffix == "xls") {
        // 处理Excel文件 - 显示进度提示
        QProgressDialog *importProgress = new QProgressDialog(this);
        importProgress->setWindowModality(Qt::ApplicationModal);
        importProgress->setWindowFlags(importProgress->windowFlags() | Qt::WindowStaysOnTopHint);
        importProgress->setCancelButton(nullptr);
        importProgress->setMinimumDuration(0);
        importProgress->setWindowTitle("导入进度");
        importProgress->setLabelText("正在导入Excel文件，请稍候...");
        importProgress->setRange(0, 0); // 不确定进度模式
        importProgress->show();
        QApplication::processEvents();

        QAxObject *excel = new QAxObject("Excel.Application");
        if (!excel || excel->isNull()) {
            delete importProgress;
            QMessageBox::warning(this, "错误", "无法启动Excel应用程序。请确保已安装Microsoft Excel。");
            if (excel) delete excel;
            return;
        }

        importProgress->setLabelText("正在连接Excel...");
        QApplication::processEvents();

        // 设置 Excel 应用程序不可见
        excel->dynamicCall("SetVisible(bool)", false);
        // 获取 Excel 应用程序的工作簿集合
        importProgress->setLabelText("正在打开Excel文件...");
        QApplication::processEvents();
        
        QAxObject *workbooks = excel->querySubObject("Workbooks");
        if (!workbooks || workbooks->isNull()) {
            delete importProgress;
            excel->dynamicCall("Quit()");
            delete excel;
            QMessageBox::warning(this, "错误", "无法访问Excel工作簿集合。");
            return;
        }

        // 打开Excel文件
        QVariant openResult = workbooks->dynamicCall("Open(const QString&)", QDir::toNativeSeparators(filePath));
        QAxObject *workbook = excel->querySubObject("ActiveWorkbook");
        if (!workbook || workbook->isNull()) {
            delete importProgress;
            excel->dynamicCall("Quit()");
            delete excel;
            QMessageBox::warning(this, "错误", "无法打开Excel文件：" + filePath);
            return;
        }

        importProgress->setLabelText("正在读取工作表数据...");
        QApplication::processEvents();

        // 获取第一个工作表
        QAxObject *worksheet = workbook->querySubObject("Worksheets(int)", 1);
        if (!worksheet || worksheet->isNull()) {
            delete importProgress;
            workbook->dynamicCall("Close(bool)", false);
            excel->dynamicCall("Quit()");
            delete excel;
            QMessageBox::warning(this, "错误", "无法访问工作表。");
            return;
        }

        // 获取已使用的范围
        QAxObject *usedRange = worksheet->querySubObject("UsedRange");
        if (!usedRange || usedRange->isNull()) {
            delete importProgress;
            delete worksheet;
            workbook->dynamicCall("Close(bool)", false);
            excel->dynamicCall("Quit()");
            delete excel;
            QMessageBox::warning(this, "错误", "工作表为空或无法读取。");
            return;
        }

        // 获取行数和列数
        QAxObject *rows = usedRange->querySubObject("Rows");
        QAxObject *columns = usedRange->querySubObject("Columns");
        int rowCount = rows ? rows->property("Count").toInt() : 0;
        int colCount = columns ? columns->property("Count").toInt() : 0;

        // 设置进度条范围（如果有数据行）
        if (rowCount > 1) {
            importProgress->setRange(0, rowCount - 1); // 减去表头行
            importProgress->setValue(0);
        }

        // 从第二行开始读取数据（第一行是表头）
        for (int row = 2; row <= rowCount; ++row) {
            // 更新进度提示
            int progress = row - 2; // 当前处理的行数（从0开始）
            importProgress->setValue(progress);
            importProgress->setLabelText(QString("正在导入第 %1/%2 条数据...").arg(progress + 1).arg(rowCount - 1));
            QApplication::processEvents();

            // 读取姓名（第1列，索引为1）
            QAxObject *nameCell = worksheet->querySubObject("Cells(int,int)", row, 1);
            QString name = nameCell ? nameCell->property("Value").toString() : "";
            if (nameCell) delete nameCell;

            // 读取组别（第2列，索引为2）
            QAxObject *groupCell = worksheet->querySubObject("Cells(int,int)", row, 2);
            int group = groupCell ? groupCell->property("Value").toInt() : 0;
            if (groupCell) delete groupCell;

            if (name.isEmpty() || group < 1 || group > 4) {
                failCount++;
                continue;
            }

            // 读取时间安排（第3列到第22列，共20个时间点）
            bool time[4][5];
            bool hasValidData = false;
            for (int col = 3; col <= 22; ++col) {
                QAxObject *timeCell = worksheet->querySubObject("Cells(int,int)", row, col);
                QVariant cellValue = timeCell ? timeCell->property("Value") : QVariant(0);
                int timeValue = cellValue.toInt();
                if (timeCell) delete timeCell;

                int timeIndex = col - 3; // 0-19
                int rowIndex = timeIndex / 5; // 0-3
                int colIndex = timeIndex % 5; // 0-4
                time[rowIndex][colIndex] = (timeValue == 1);
                if (timeValue == 0 || timeValue == 1) {
                    hasValidData = true;
                }
            }

            if (!hasValidData) {
                failCount++;
                continue;
            }

            // 查找对应的队员
            bool found = false;
            auto& members = flagGroup.getGroupMembers(group);
            for (auto& member : members) {
                if (QString::fromStdString(member.getName()) == name) {
                    member.setTime(time);
                    found = true;
                    successCount++;
                    break;
                }
            }
            if (!found) {
                failCount++;
            }
        }

        importProgress->setLabelText("正在完成导入...");
        QApplication::processEvents();

        // 清理资源
        importProgress->setLabelText("正在关闭Excel文件...");
        QApplication::processEvents();
        
        if (rows) delete rows;
        if (columns) delete columns;
        delete usedRange;
        delete worksheet;
        workbook->dynamicCall("Close(bool)", false); // 不保存更改
        delete workbook;
        excel->dynamicCall("Quit()");
        delete excel;
        
        // 关闭进度对话框
        importProgress->close();
        delete importProgress;
    } else {
        QMessageBox::warning(this, "错误", "不支持的文件格式：" + suffix);
        return;
    }

    // 更新当前显示的队员信息
    if (currentSelectedPerson) {
        updateAttendanceButtons(*currentSelectedPerson);
    }

    // 更新所有组的ListView
    for (int i = 1; i <= 4; ++i) {
        updateListView(i);
    }

    QString message = QString("导入完成！\n成功：%1 条\n失败：%2 条").arg(successCount).arg(failCount);
    QMessageBox::information(this, "导入结果", message);
}

// 全部可用
void SystemWindow::onSetAllAvailableButtonClicked()
{
    if (!currentSelectedPerson) {
        QMessageBox::warning(this, "提示", "请先选择一个队员");
        return;
    }

    // 设置所有时间为可用
    bool time[4][5];
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 5; ++j) {
            time[i][j] = true;
        }
    }
    currentSelectedPerson->setTime(time);

    // 更新UI显示
    updateAttendanceButtons(*currentSelectedPerson);
    QMessageBox::information(this, "成功", "已设置所有时间为可用");
}

// 全部不可用
void SystemWindow::onSetAllUnavailableButtonClicked()
{
    if (!currentSelectedPerson) {
        QMessageBox::warning(this, "提示", "请先选择一个队员");
        return;
    }

    // 设置所有时间为不可用
    bool time[4][5];
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 5; ++j) {
            time[i][j] = false;
        }
    }
    currentSelectedPerson->setTime(time);

    // 更新UI显示
    updateAttendanceButtons(*currentSelectedPerson);
    QMessageBox::information(this, "成功", "已设置所有时间为不可用");
}

// 从表格管理界面导入空闲时间
void SystemWindow::onImportTimeFromTaskButtonClicked()
{
    // 直接调用导入空闲时间函数
    onImportTimeButtonClicked();
}
