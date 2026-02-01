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
#include <QDebug>
#include <QTimer>
#include <QInputDialog>
#include <QPushButton>
#include <QVBoxLayout>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QDialog>
#include <QTextEdit>
#include <QLabel>
#include <QEvent>
#include "systemwindow.h"
#include "fileFunction.h"
#include "dataFunction.h"
#include "encryptedFileManager.h"

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

    // 在窗口启动时读取文件（优先使用加密格式，如果不存在则尝试旧格式）
    QString encryptedFilename = filename;
    encryptedFilename.replace(".txt", ".dat"); // 使用.dat扩展名表示加密文件
    
    // 确保data目录存在
    QFileInfo fileInfo(filename);
    QDir dir = fileInfo.absoluteDir();
    if (!dir.exists()) {
        dir.mkpath("."); // 创建目录
    }
    
    // 尝试读取加密文件
    bool loaded = false;
    QFile encryptedFile(encryptedFilename);
    if (encryptedFile.exists()) {
        loaded = EncryptedFileManager::loadFromFile(flagGroup, encryptedFilename);
        if (loaded) {
            qDebug() << "成功从加密文件读取数据：" << encryptedFilename;
        } else {
            qDebug() << "加密文件读取失败，尝试读取旧格式：" << encryptedFilename;
        }
    } else {
        qDebug() << "加密文件不存在，尝试读取旧格式：" << encryptedFilename;
    }
    
    // 如果加密文件不存在或读取失败，尝试读取旧格式文件（向后兼容）
    if (!loaded) {
        QFile oldFile(filename);
        if (oldFile.exists()) {
            FlagGroupFileManager::loadFromFile(flagGroup, filename);
            qDebug() << "从旧格式文件读取数据：" << filename;
            // 如果旧文件存在，自动转换为新格式
            EncryptedFileManager::saveToFile(flagGroup, encryptedFilename);
            qDebug() << "已将旧格式转换为加密格式：" << encryptedFilename;
        } else {
            qDebug() << "数据文件不存在，使用空数据：" << filename;
        }
    }

    // 排表历史记录：使用单独文件 ./data/schedule_history.dat，与队员数据分离
    QString historyPath = QFileInfo(filename).absolutePath() + "/schedule_history.dat";
    historyManager.setHistoryFilePath(historyPath);
    if (historyManager.loadFromFile(historyPath)) {
        qDebug() << "已加载排表历史记录：" << historyPath;
    }

    // 刚启动时，内存中的数据与文件一致（或为空初始状态），认为没有未保存修改
    dataSaved = true;
    hasUnsavedChanges = false;
    discardWithoutSave = false;

    // 执勤管理界面
    // 连接按钮和复选框的信号与槽
    connect(ui->tabulateButton, &QPushButton::clicked, this, &SystemWindow::onTabulateButtonClicked); // 排表按钮点击事件
    connect(ui->clearButton, &QPushButton::clicked, this, &SystemWindow::onHistoryButtonClicked); // 查看历史记录按钮点击事件
    connect(ui->alterButton, &QPushButton::clicked, this, &SystemWindow::onRestoreScheduleButtonClicked); // 恢复排班按钮点击事件
    connect(ui->deriveButton, &QPushButton::clicked, this, &SystemWindow::onExportButtonClicked); // 导出表格按钮点击事件
    connect(ui->importTimeFromTask_pushButton, &QPushButton::clicked, this, &SystemWindow::onImportTimeFromTaskButtonClicked); // 导入空闲时间按钮点击事件
    // 导出表格在初始时取消交互
    ui->deriveButton->setEnabled(false);
    // 历史记录按钮始终可用
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
    connect(ui->njh_total_times_spinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &SystemWindow::onInfoLineEditChanged);
    connect(ui->dxy_total_times_spinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &SystemWindow::onInfoLineEditChanged);

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
    
    // 连接应用程序退出信号，确保在任何情况下都能保存数据
    connect(qApp, &QApplication::aboutToQuit, this, &SystemWindow::onApplicationAboutToQuit);
    
    // 初始化管理员权限状态（默认为普通模式）
    // 使用快捷键 Ctrl+Shift+A 触发管理员登录（隐蔽方式）
    isAdminMode = false;
    updateAdminButtonsState();
    
    // 初始化彩蛋相关
    clickTimer = new QTimer(this);
    clickTimer->setSingleShot(true);
    connect(clickTimer, &QTimer::timeout, this, [this]() {
        instructionTabClickCount = 0; // 超时后重置计数
    });
    
    // 为使用说明文本框安装事件过滤器，用于检测双击
    ui->instructionText->installEventFilter(this);
    ui->instructionText->viewport()->installEventFilter(this);
}
SystemWindow::~SystemWindow()
{
    delete exportProgress;
    delete ui;
}
//关闭窗口事件
void SystemWindow::closeEvent(QCloseEvent *event)
{
    // 如果数据已经保存，直接关闭
    if (dataSaved) {
        event->accept();
        return;
    }
    
    // 弹出提示窗口，询问是否保存（按钮文字使用中文）
    QMessageBox msgBox(this);
    msgBox.setWindowTitle("关闭系统");
    msgBox.setText("检测到数据可能未保存，是否保存后关闭？");
    msgBox.setInformativeText("如果选择“不保存”，本次修改的队员数据及排表历史记录将全部丢失。");
    msgBox.setIcon(QMessageBox::Question);

    // 使用自定义中文按钮
    QPushButton *saveButton = msgBox.addButton("保存", QMessageBox::AcceptRole);
    QPushButton *discardButton = msgBox.addButton("不保存", QMessageBox::DestructiveRole);
    msgBox.setDefaultButton(saveButton);

    msgBox.exec();

    QAbstractButton *clicked = msgBox.clickedButton();
    if (clicked == saveButton) {
        // 保存并关闭
        if (saveDataToFile()) {
            event->accept();
        } else {
            // 保存失败，询问是否仍要关闭
            QMessageBox::StandardButton retry = QMessageBox::question(
                this,
                "保存失败",
                "无法保存数据文件！\n是否仍要关闭程序？\n（数据可能会丢失）",
                QMessageBox::Yes | QMessageBox::No,
                QMessageBox::No
            );
            if (retry == QMessageBox::Yes) {
                event->accept();
            } else {
                event->ignore();
            }
        }
    } else if (clicked == discardButton) {
        // 用户明确选择“不保存直接关闭”
        discardWithoutSave = true;      // 后续 aboutToQuit 不再尝试自动保存
        hasUnsavedChanges = false;      // 认为当前未保存修改已被放弃
        dataSaved = false;             // 内存与文件不一致，但用户已同意放弃
        event->accept();
    } else {
        // 取消关闭或点击关闭按钮
        event->ignore();
    }
}

// 重写键盘事件，用于快捷键触发管理员登录
void SystemWindow::keyPressEvent(QKeyEvent *event)
{
    // 检测 Ctrl+Shift+A 组合键
    if (event->modifiers() == (Qt::ControlModifier | Qt::ShiftModifier) && 
        event->key() == Qt::Key_A) {
        onAdminLoginClicked();
        event->accept();
        return;
    }
    
    // 其他按键事件交给父类处理
    QMainWindow::keyPressEvent(event);
}

// 保存数据到文件
bool SystemWindow::saveDataToFile()
{
    // 确保data目录存在
    QString encryptedFilename = filename;
    encryptedFilename.replace(".txt", ".dat");
    
    QFileInfo fileInfo(encryptedFilename);
    QDir dir = fileInfo.absoluteDir();
    if (!dir.exists()) {
        dir.mkpath(".");
    }
    
    // 保存文件（使用加密格式）
    bool saved = EncryptedFileManager::saveToFile(flagGroup, encryptedFilename);
    
    if (saved) {
        dataSaved = true;
        hasUnsavedChanges = false;   // 所有修改已写入文件
        discardWithoutSave = false;  // 当前状态是“已保存”，不再视为放弃保存
        qDebug() << "数据已保存：" << encryptedFilename;
        return true;
    } else {
        qDebug() << "数据保存失败：" << encryptedFilename;
        return false;
    }
}

// 应用程序即将退出时的处理（作为最后保障）
void SystemWindow::onApplicationAboutToQuit()
{
    if (discardWithoutSave) {
        // 用户已选择「不保存」：不写队员数据，不写排表历史；本次会话的修改全部丢弃
        return;
    }
    // 未选择「不保存」：视情况保存队员数据，并始终保存排表历史
    if (hasUnsavedChanges) {
        qDebug() << "检测到程序即将退出，尝试自动保存数据...";
        if (saveDataToFile()) {
            qDebug() << "数据已自动保存";
        } else {
            qDebug() << "自动保存失败，数据可能丢失";
        }
    }
    if (historyManager.saveToFile()) {
        qDebug() << "排表历史已保存";
    }
}

// 标记数据已被修改
void SystemWindow::markDataChanged()
{
    hasUnsavedChanges = true;
    dataSaved = false;
    discardWithoutSave = false;
}

//值周管理界面函数实现
void SystemWindow::onTabulateButtonClicked() {

    // 制表按钮
    // 关键修复：每次排表前都删除旧的 manager 并重新创建，确保 availableMembers 是最新的
    if (manager) {
        delete manager;
        manager = nullptr;
    }
    
    manager = new SchedulingManager(flagGroup);
    connect(manager, &SchedulingManager::schedulingWarning, this, &SystemWindow::handleSchedulingWarning);  // 连接警告信号与发送警告信息的槽函数
    connect(manager, &SchedulingManager::schedulingFinished, this, [this]() {
        ui->tabulateButton->setEnabled(false);
        ui->deriveButton->setEnabled(true);
        updateTableWidget(*manager); // 制表操作
        updateTextEdit(*manager); // 更新制表结果文本域
        
        // 保存历史记录
        QString mode;
        if (ui->normal_mode_radioButton->isChecked()) {
            mode = "常规模式";
        } else if (ui->supervisory_mode_radioButton->isChecked()) {
            mode = "监督模式";
        } else if (ui->DXY_only_twice_mode_radioButton->isChecked()) {
            mode = "东西院仅两次模式";
        } else if (ui->custom_mode_radioButton->isChecked()) {
            mode = "自定义模式";
        } else {
            mode = "常规模式";
        }
        historyManager.addHistory(flagGroup, *manager, mode, finalText_excel);
        
        delete manager;
        manager = nullptr;
    });

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
    if (manager->getAvailableMembers().size() < 12) {
        QMessageBox::warning(nullptr,"排表错误警告","现在国旗班12个队员都凑不出来了吗:(");
    } else {
        manager->schedule();
        // 排表会修改队员总次数等信息，属于需要保存的更改
        markDataChanged();
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
        resultText += QString::fromStdString(member->getName()) +
                      " 本周工作次数: " + QString::number(member->getTimes()) +
                      " 总工作次数: " + QString::number(member->getAll_times()) +
                      " （南鉴湖累计: " + QString::number(member->getNJHAllTimes()) +
                      "，东西院累计: " + QString::number(member->getDXYAllTimes()) + "）\n";
    }
    // 拼接警告信息和排班结果文本
    QString finalText = warningMessages + resultText;
    finalText_excel = finalText;
    // 设置最终文本到文本编辑框
    ui->timesResult->setPlainText(finalText);
    // 清空警告信息，以便下次排表使用
    warningMessages.clear();
}
void SystemWindow::onHistoryButtonClicked() {
    // 查看历史记录按钮
    HistoryDialog* dialog = new HistoryDialog(&historyManager, &flagGroup, this);
    connect(dialog, &HistoryDialog::restoreHistoryRequested, this, &SystemWindow::restoreFromHistory);
    dialog->exec();
    delete dialog;
}

void SystemWindow::restoreFromHistory(int historyIndex) {
    // 从历史记录恢复状态
    const ScheduleHistoryItem* item = historyManager.getHistory(historyIndex);
    if (!item) {
        QMessageBox::warning(this, "错误", "无法获取历史记录信息。");
        return;
    }
    
    // 恢复队员信息
    flagGroup = item->flagGroupSnapshot;
    
    // 恢复排班表（需要重新创建SchedulingManager并设置排班表）
    // 先删除旧的manager，确保下次排班时能重新创建
    if (manager) {
        delete manager;
        manager = nullptr;
    }
    
    // 创建临时manager用于恢复排班表显示
    SchedulingManager* tempManager = new SchedulingManager(flagGroup);
    
    // 根据历史记录中的排班表信息恢复排班状态
    std::vector<std::vector<std::vector<Person*>>> scheduleTable;
    scheduleTable.resize(item->scheduleTable.size());
    for (size_t slot = 0; slot < item->scheduleTable.size(); ++slot) {
        scheduleTable[slot].resize(item->scheduleTable[slot].size());
        for (size_t location = 0; location < item->scheduleTable[slot].size(); ++location) {
            scheduleTable[slot][location].resize(item->scheduleTable[slot][location].size());
            for (size_t position = 0; position < item->scheduleTable[slot][location].size(); ++position) {
                const SchedulePosition& pos = item->scheduleTable[slot][location][position];
                Person* person = pos.findPerson(flagGroup);
                scheduleTable[slot][location][position] = person;
            }
        }
    }
    tempManager->setScheduleTable(scheduleTable);
    
    // 使用临时manager更新UI
    updateTableWidget(*tempManager);
    
    // 删除临时manager，确保下次排班时能重新创建
    delete tempManager;
    manager = nullptr;
    ui->timesResult->setPlainText(item->scheduleText);
    finalText_excel = item->scheduleText;
    
    // 更新按钮状态
    ui->tabulateButton->setEnabled(false);
    ui->deriveButton->setEnabled(true);
    
    // 刷新队员列表显示
    for (int i = 1; i <= 4; ++i) {
        updateListView(i);
    }
    
    // 标记数据已更改
    markDataChanged();
    
    QMessageBox::information(this, "恢复成功", "已成功恢复到选中的历史记录状态。");
}
void SystemWindow::onRestoreScheduleButtonClicked() {
    // 恢复排班按钮：重新启用排班、关闭导出，允许用户再次排班
    ui->tabulateButton->setEnabled(true);
    ui->deriveButton->setEnabled(false);
    QMessageBox::information(this, "恢复排班", "已恢复排班功能，可以重新进行排班操作。");
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
    
    // 检查文件是否已存在且可能被占用
    QFileInfo fileInfo(filePath);
    if (fileInfo.exists()) {
        // 尝试打开文件检查是否被占用
        QFile testFile(filePath);
        if (!testFile.open(QIODevice::ReadWrite)) {
            // 文件被占用，询问用户
            QMessageBox::StandardButton reply = QMessageBox::question(
                this,
                "文件被占用",
                QString("文件 \"%1\" 可能正在被其他程序使用（如Excel）。\n\n是否仍要尝试覆盖？\n（如果文件被占用，导出可能会失败）").arg(fileInfo.fileName()),
                QMessageBox::Yes | QMessageBox::No,
                QMessageBox::No
            );
            if (reply == QMessageBox::No) {
                return; // 用户取消导出
            }
        } else {
            testFile.close(); // 文件未被占用，可以继续
        }
    }

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

    // 强制更新UI，确保初始提示显示，增加事件处理次数
    for (int i = 0; i < 5; ++i) {
        QApplication::processEvents(QEventLoop::AllEvents);
    }

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
                if (i % 5 == 0) { // 每5行处理一次事件，增加频率
                    for (int j = 0; j < 2; ++j) {
                        QApplication::processEvents(QEventLoop::AllEvents);
                    }
                    if (exportProgress->wasCanceled()) {
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
                // 每行处理一次事件，让进度条更流畅
                if (row % 1 == 0) {
                    QApplication::processEvents(QEventLoop::AllEvents);
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
            // 如果目标文件已存在，先尝试删除（避免SaveAs时的覆盖提示卡死）
            QFile targetFile(filePath);
            if (targetFile.exists()) {
                // 尝试删除旧文件，如果失败说明文件被占用
                if (!targetFile.remove()) {
                    qDebug() << "警告：无法删除旧文件，可能被占用：" << filePath;
                    // 继续尝试SaveAs，Excel可能会提示覆盖
                }
            }
            
            // 将工作簿保存到用户指定的路径
            // 使用DisplayAlerts=false来避免覆盖提示对话框导致卡死
            excel->dynamicCall("SetDisplayAlerts(bool)", false);
            QVariant saveResult = workbook->dynamicCall("SaveAs(const QString&)", QDir::toNativeSeparators(filePath));
            excel->dynamicCall("SetDisplayAlerts(bool)", true);
            
            // 检查保存是否成功
            bool saveSuccess = false;
            if (saveResult.isNull() || !saveResult.toBool()) {
                qDebug() << "Excel保存可能失败：" << filePath;
            } else {
                // 验证文件是否真的存在
                QFileInfo savedFile(filePath);
                if (savedFile.exists() && savedFile.size() > 0) {
                    saveSuccess = true;
                }
            }
            
            // 关闭工作簿
            workbook->dynamicCall("Close()");
            processStep("保存工作簿", exportProgress);
            
            // 退出 Excel 应用程序
            excel->dynamicCall("Quit()");
            // 释放 Excel 应用程序对象的内存
            delete excel;
            
            // 导出完成后恢复控件状态并提示
            if (exportProgress) {
                exportProgress->hide();
            }
            
            // 只有在成功保存后才显示成功消息并恢复排班按钮
            if (saveSuccess) {
                QMessageBox::information(this, "导出完成", "表格已成功导出至：\n" + filePath, QMessageBox::Ok);
                // 恢复排班按钮的交互功能，关闭导出按钮
                ui->tabulateButton->setEnabled(true);
                ui->deriveButton->setEnabled(false);
            } else {
                QMessageBox::warning(this, "导出失败", 
                    "表格导出可能失败，请检查文件路径和权限。\n\n"
                    "文件路径：" + filePath + "\n\n"
                    "建议解决方案：\n"
                    "1. 可以通过\"查看历史记录功能\"回退到之前的记录，然后重新导出\n"
                    "2. 或者关闭程序后重新打开，重新进行排班和导出操作",
                    QMessageBox::Ok);
            }
        } else {
            // workbook创建失败
            excel->dynamicCall("Quit()");
            delete excel;
            if (exportProgress) {
                exportProgress->hide();
            }
            QMessageBox::warning(this, "导出失败", 
                "无法创建工作簿，导出失败。\n\n"
                "建议解决方案：\n"
                "1. 可以通过\"查看历史记录功能\"回退到之前的记录，然后重新导出\n"
                "2. 或者关闭程序后重新打开，重新进行排班和导出操作",
                QMessageBox::Ok);
        }
    } else {
        // Excel启动失败
        if (exportProgress) {
            exportProgress->hide();
        }
        QMessageBox::warning(this, "导出失败", 
            "无法启动Excel应用程序，请确保已安装Microsoft Excel。\n\n"
            "建议解决方案：\n"
            "1. 安装或修复Microsoft Excel\n"
            "2. 如果Excel已安装但仍无法启动，可以通过\"查看历史记录\"功能回退到之前的记录，然后重新导出\n"
            "3. 或者关闭程序后重新打开，重新进行排班和导出操作",
            QMessageBox::Ok);
    }

}
// 自定义进度更新函数 - 优化版，增加更多事件处理
void SystemWindow::processStep(const QString& stepName, QProgressDialog* progress) {
    if (progress && progress->isVisible()) {
        progress->setLabelText("正在导出表格 - " + stepName + "...");
        // 多次调用 processEvents，让进度条动画更流畅
        for (int i = 0; i < 3; ++i) {
            QApplication::processEvents(QEventLoop::AllEvents);
        }
    }
}


//队员管理界面函数实现
void SystemWindow::onGroupAddButtonClicked(int groupIndex)
{
    //添加队员按钮点击事件
    // 检查组号是否有效
    if (groupIndex < 1 || groupIndex > 4) {
        QMessageBox::warning(this, "错误", "无效的组别");
        return;
    }
    
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

    // 生成唯一的默认名字（在当前组内唯一即可）
    std::string defaultNameBase = "未命名队员";
    int counter = 1;
    std::string defaultName = defaultNameBase + std::to_string(counter);

    // 获取当前组的所有队员名字
    const auto& members = flagGroup.getGroupMembers(groupIndex);
    std::vector<std::string> existingNames;
    for (const auto& member : members) {
        existingNames.push_back(member.getName());
    }

    // 检查名字是否在当前组内重复，若重复则递增计数器
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
    
    // 数据已发生更改
    markDataChanged();
}
void SystemWindow::onGroupDeleteButtonClicked(int groupIndex)
{
    //删除队员按钮点击事件
    qDebug() << "\n========== [SystemWindow::onGroupDeleteButtonClicked] 删除队员操作开始 ==========";
    qDebug() << "触发删除的组别:" << groupIndex;
    
    QListView* listView = nullptr;
    //判断是哪个组发出的信号，以及信号情况
    switch (groupIndex) {
    case 1: listView = ui->group1_info_listView; break;
    case 2: listView = ui->group2_info_listView; break;
    case 3: listView = ui->group3_info_listView; break;
    case 4: listView = ui->group4_info_listView; break;
    }
    //如果出现非法组号，退出
    if (!listView) {
        qDebug() << "错误：listView为空";
        return;
    }
    
    // 检查是否有选择模型
    if (!listView->selectionModel()) {
        qDebug() << "错误：selectionModel为空";
        return;
    }
    
    QModelIndexList selectedIndexes = listView->selectionModel()->selectedIndexes();
    //不存在被选定的队员
    if (selectedIndexes.isEmpty()) {
        qDebug() << "提示：未选择任何队员";
        QMessageBox::information(this, "提示", "请先选择要删除的队员");
        return;
    }
    
    //删除前的确认弹窗
    QMessageBox::StandardButton reply = QMessageBox::question(this, "确认删除", "是否删除该队员？", QMessageBox::Yes | QMessageBox::No);
    //删除对应队员
    if (reply == QMessageBox::Yes) {
        qDebug() << "用户确认删除";
        
        int row = selectedIndexes.first().row();
        qDebug() << "选中的行索引:" << row;
        
        const auto& members = flagGroup.getGroupMembers(groupIndex);
        qDebug() << "组" << groupIndex << "当前队员数量:" << members.size();
        
        // 检查行索引是否有效
        if (row < 0 || static_cast<std::vector<Person>::size_type>(row) >= members.size()) {
            qDebug() << "错误：行索引无效";
            QMessageBox::warning(this, "错误", "选择的队员索引无效");
            return;
        }
        
        // 获取待删除队员的姓名（用于后续判断）
        std::string personNameToDelete = members[row].getName();
        int personGroupToDelete = members[row].getGroup();
        qDebug() << "待删除队员姓名:" << QString::fromStdString(personNameToDelete);
        qDebug() << "待删除队员的组别属性:" << personGroupToDelete;
        
        // 打印删除前所有组的队员情况
        qDebug() << "\n--- 删除前各组队员情况 ---";
        for (int i = 1; i <= 4; ++i) {
            const auto& grp = flagGroup.getGroupMembers(i);
            qDebug() << "组" << i << "队员数量:" << grp.size();
            for (size_t j = 0; j < grp.size(); ++j) {
                qDebug() << "  [" << j << "]" << QString::fromStdString(grp[j].getName()) 
                         << "(组别属性:" << grp[j].getGroup() << ")";
            }
        }
        
        // 检查删除的是否是当前显示的队员（通过姓名和组别判断）
        if (currentSelectedPerson && 
            currentSelectedPerson->getName() == personNameToDelete && 
            currentSelectedPerson->getGroup() == groupIndex) {
            qDebug() << "删除的是当前显示的队员，清空信息显示区";
            currentSelectedPerson = nullptr; // 先重置指针，防止意外操作
            clearMemberInfoDisplay(); // 再清空信息显示区
        }
        
        // 创建待删除队员的副本（只需要姓名即可定位）
        Person personToRemove = members[row];
        qDebug() << "创建待删除队员副本，姓名:" << QString::fromStdString(personToRemove.getName());
        qDebug() << "副本的组别属性:" << personToRemove.getGroup();
        
        // 调用 Flag_group 的删除成员方法（在指定组内通过姓名删除）
        qDebug() << "\n调用 flagGroup.removePersonFromGroup()...";
        flagGroup.removePersonFromGroup(personToRemove, groupIndex);
        
        // 打印删除后所有组的队员情况
        qDebug() << "\n--- 删除后各组队员情况 ---";
        for (int i = 1; i <= 4; ++i) {
            const auto& grp = flagGroup.getGroupMembers(i);
            qDebug() << "组" << i << "队员数量:" << grp.size();
            for (size_t j = 0; j < grp.size(); ++j) {
                qDebug() << "  [" << j << "]" << QString::fromStdString(grp[j].getName()) 
                         << "(组别属性:" << grp[j].getGroup() << ")";
            }
        }
        
        // 更新对应组的ListView组员标签信息
        qDebug() << "\n更新组" << groupIndex << "的ListView...";
        updateListView(groupIndex);
        
        // 数据已发生更改
        markDataChanged();
        
        qDebug() << "========== 删除队员操作结束 ==========\n";
    } else {
        qDebug() << "用户取消删除";
    }
}
// 新增的清空信息显示区函数
void SystemWindow::clearMemberInfoDisplay()
{
    qDebug() << "[SystemWindow::clearMemberInfoDisplay] 开始清空信息显示区";
    
    // 关键修复：在清空UI之前，先设置标志位，防止触发信号导致意外操作
    isShowingInfo = true;
    
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
    ui->group_combobox->setCurrentIndex(0);  // 这行会触发 onGroupComboBoxChanged 信号！
    ui->total_times_spinBox->setValue(0);

    // 清空执勤时间按钮状态
    QList<QAbstractButton*> attendanceButtons = ui->availableTime_groupBox->findChildren<QAbstractButton*>();
    for (QAbstractButton* button : attendanceButtons) {
        if (!button->objectName().endsWith("all_pushButton")) {
            button->setChecked(false);
        }
    }
    
    // 恢复标志位
    isShowingInfo = false;
    
    qDebug() << "[SystemWindow::clearMemberInfoDisplay] 清空完成";
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
    // 该组的是否值周状态已更改
    markDataChanged();
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
    if (!currentSelectedPerson || isShowingInfo) {
        return;
    }
    
    // 检查当前选中的队员是否仍然存在于对应的组中
    int groupIndex = currentSelectedPerson->getGroup();
    Person* foundPerson = flagGroup.findPersonInGroup(*currentSelectedPerson, groupIndex);
    
    if (!foundPerson) {
        // 队员已不存在，清空显示
        currentSelectedPerson = nullptr;
        clearMemberInfoDisplay();
        QMessageBox::warning(this, "警告", "当前队员已被删除或移动，请重新选择");
        return;
    }
    
    updatePersonInfo(*currentSelectedPerson);
    // 基础信息修改
    markDataChanged();
}
void SystemWindow::onGroupComboBoxChanged(int newGroupIndex)
{
    // 独立的修改组别函数
    if (!currentSelectedPerson || isShowingInfo) {
        return;
    }
    
    int oldGroupIndex = currentSelectedPerson->getGroup();//值为1~4
    
    // 规避并未修改组别引发多余操作
    if((oldGroupIndex - 1) == newGroupIndex) {
        return;
    }
    
    newGroupIndex += 1; // combobox 索引从 0 开始，组索引从 1 开始
    
    // 检查新组号是否有效
    if (newGroupIndex < 1 || newGroupIndex > 4) {
        QMessageBox::warning(this, "错误", "无效的组别");
        // 恢复原来的组别显示
        ui->group_combobox->setCurrentIndex(oldGroupIndex - 1);
        return;
    }
    
    // 检查新组中是否已存在同名队员
    const auto& newGroupMembers = flagGroup.getGroupMembers(newGroupIndex);
    std::string currentName = currentSelectedPerson->getName();
    for (const auto& member : newGroupMembers) {
        if (member.getName() == currentName) {
            QMessageBox::warning(this, "错误",
                QString("目标组别中已存在名为\"%1\"的队员，无法切换组别。\n请先修改队员姓名或删除目标组中的同名队员。")
                .arg(QString::fromStdString(currentName)));
            // 恢复原来的组别显示
            ui->group_combobox->setCurrentIndex(oldGroupIndex - 1);
            return;
        }
    }
    
    // 创建当前队员的副本，避免指针失效
    Person personCopy = *currentSelectedPerson;
    
    // 更新队员的组别信息
    personCopy.setGroup(newGroupIndex);
    
    bool isChecked = false;
    switch (newGroupIndex) {
    case 1: isChecked = ui->group1_iswork_radioButton->isChecked(); break;
    case 2: isChecked = ui->group2_iswork_radioButton->isChecked(); break;
    case 3: isChecked = ui->group3_iswork_radioButton->isChecked(); break;
    case 4: isChecked = ui->group4_iswork_radioButton->isChecked(); break;
    }
    personCopy.setIsWork(isChecked);
    
    // 先从旧组中删除队员（通过姓名在指定组内删除）
    flagGroup.removePersonFromGroup(*currentSelectedPerson, oldGroupIndex);
    
    // 将队员添加到新组中（使用更新后的队员信息）
    flagGroup.addPersonToGroup(personCopy, newGroupIndex);
    
    // 更新两个组的ListView显示
    updateListView(oldGroupIndex);
    updateListView(newGroupIndex);
    
    // 在新组中查找刚添加的队员，更新当前选中的队员指针
    Person* person = flagGroup.findPersonInGroup(personCopy, newGroupIndex);
    if (person) {
        currentSelectedPerson = person;
    } else {
        // 如果找不到，清空当前选中
        currentSelectedPerson = nullptr;
        clearMemberInfoDisplay();
        QMessageBox::warning(this, "警告", "组别切换后无法定位队员，请重新选择");
    }
    
    // 组别发生变化
    markDataChanged();
}
void SystemWindow::updateListView(int groupIndex)
{
    // 更新队员标签界面
    qDebug() << "[SystemWindow::updateListView] 更新组" << groupIndex << "的ListView";
    
    QListView* listView = nullptr;
    switch (groupIndex) {
    case 1: listView = ui->group1_info_listView; break;
    case 2: listView = ui->group2_info_listView; break;
    case 3: listView = ui->group3_info_listView; break;
    case 4: listView = ui->group4_info_listView; break;
    }
    if (!listView) {
        qDebug() << "  错误：listView为空";
        return;
    }
    
    const auto& members = flagGroup.getGroupMembers(groupIndex);
    qDebug() << "  组" << groupIndex << "当前队员数量:" << members.size();
    
    QStringList memberNames;
    for (const auto& member : members) {
        QString name = QString::fromStdString(member.getName());
        memberNames << name;
        qDebug() << "    - " << name << "(组别属性:" << member.getGroup() << ")";
    }
    
    // 使用 QStringListModel 替代 QStandardItemModel
    QStringListModel *model = new QStringListModel();//创建一个 QStringListModel 对象 model，它是 QAbstractItemModel 的子类，专门用于处理字符串列表数据
    model->setStringList(memberNames);//调用 model 的 setStringList 方法，将 memberNames 列表中的数据设置到模型中。
    listView->setModel(model);//调用 listView 的 setModel 方法，将 model 设置为 listView 的数据模型。这样，listView 就会显示 model 中的数据，即指定组别的队员姓名。
    // 关键设置：禁用所有编辑触发，但保留点击功能
    listView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    // 可选：设置选择模式，允许点击选择
    listView->setSelectionMode(QAbstractItemView::SingleSelection);
    
    qDebug() << "  ListView更新完成";
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
    ui->njh_total_times_spinBox->setValue(person.getNJHAllTimes());
    ui->dxy_total_times_spinBox->setValue(person.getDXYAllTimes());
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
    // 总执勤次数不允许在“队员管理”界面手动修改，始终以当前队员数据为准
    int all_times = ui->total_times_spinBox->value();
    int njh_all_times = ui->njh_total_times_spinBox->value();
    int dxy_all_times = ui->dxy_total_times_spinBox->value();
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
                     time, person.getTimes(), all_times, njh_all_times, dxy_all_times);
    
    // 如果修改了姓名，需要检查同组内是否有重名
    if (name.toStdString() != person.getName()) {
        const auto& members = flagGroup.getGroupMembers(person.getGroup());
        for (const auto& member : members) {
            // 跳过当前队员自己
            if (member.getName() == person.getName()) {
                continue;
            }
            // 检查是否与其他队员重名
            if (member.getName() == name.toStdString()) {
                QMessageBox::warning(nullptr, "错误", 
                    QString("当前组别中已存在名为\"%1\"的队员，请使用其他姓名。").arg(name));
                // 恢复原来的姓名
                ui->name_lineEdit->setText(QString::fromStdString(person.getName()));
                return;
            }
        }
    }
    
    // 更新 flagGroup 中对应队员的信息
    flagGroup.modifyPersonInGroup(person, newPerson, person.getGroup());
    // 更新对应组的 ListView 显示
    updateListView(person.getGroup());
    
    // 更新当前选中的队员指针（因为姓名可能已改变，需要重新查找）
    Person* updatedPerson = flagGroup.findPersonInGroup(newPerson, person.getGroup());
    if (updatedPerson) {
        currentSelectedPerson = updatedPerson;
    }

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
    if (!currentSelectedPerson || !button) {
        return;
    }
    
    // 检查当前选中的队员是否仍然存在
    int groupIndex = currentSelectedPerson->getGroup();
    Person* foundPerson = flagGroup.findPersonInGroup(*currentSelectedPerson, groupIndex);
    
    if (!foundPerson) {
        // 队员已不存在，清空显示
        currentSelectedPerson = nullptr;
        clearMemberInfoDisplay();
        QMessageBox::warning(this, "警告", "当前队员已被删除或移动，请重新选择");
        return;
    }
    
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
        currentSelectedPerson->setTime(row, column, button->isChecked());
        // 出勤时间更改
        markDataChanged();
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
    // 全选更改出勤时间
    markDataChanged();
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
        // 检查文件是否被占用
        QFile testFile(filePath);
        if (!testFile.open(QIODevice::ReadOnly)) {
            QMessageBox::warning(this, "文件被占用", 
                QString("文件 \"%1\" 可能正在被其他程序使用（如Excel）。\n\n请先关闭该文件，然后重试。").arg(QFileInfo(filePath).fileName()));
            return;
        }
        testFile.close();
        
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
        
        // 设置Excel不显示警告对话框，避免卡死
        excel->dynamicCall("SetDisplayAlerts(bool)", false);

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

        // 打开Excel文件（ReadOnly模式，避免文件被占用）
        QVariant openResult = workbooks->dynamicCall("Open(const QString&, bool)", QDir::toNativeSeparators(filePath), true);
        QAxObject *workbook = excel->querySubObject("ActiveWorkbook");
        if (!workbook || workbook->isNull()) {
            delete importProgress;
            excel->dynamicCall("SetDisplayAlerts(bool)", true);
            excel->dynamicCall("Quit()");
            delete excel;
            QMessageBox::warning(this, "错误", "无法打开Excel文件：" + filePath + "\n\n文件可能被其他程序占用，请先关闭该文件。");
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
        excel->dynamicCall("SetDisplayAlerts(bool)", true); // 恢复警告显示
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
    // 若有成功导入的数据，则认为数据已更改
    if (successCount > 0) {
        markDataChanged();
    }
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
    // 出勤时间更改
    markDataChanged();
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
    // 出勤时间更改
    markDataChanged();
}

// 从表格管理界面导入空闲时间
void SystemWindow::onImportTimeFromTaskButtonClicked()
{
    // 直接调用导入空闲时间函数
    onImportTimeButtonClicked();
}

// 管理员权限相关函数实现
void SystemWindow::onAdminLoginClicked()
{
    // 如果已经是管理员模式，则退出管理员模式
    if (isAdminMode) {
        QMessageBox::StandardButton reply = QMessageBox::question(
            this,
            "退出管理员模式",
            "确定要退出管理员模式吗？",
            QMessageBox::Yes | QMessageBox::No,
            QMessageBox::No
        );
        
        if (reply == QMessageBox::Yes) {
            setAdminMode(false);
            QMessageBox::information(this, "提示", "已退出管理员模式");
        }
        return;
    }
    
    // 弹出密码输入对话框（中文提示）
    bool ok;
    QInputDialog inputDialog(this);
    inputDialog.setWindowTitle("管理员登录");
    inputDialog.setLabelText("请输入管理员密码：");
    inputDialog.setTextEchoMode(QLineEdit::Password);
    inputDialog.setOkButtonText("确定");
    inputDialog.setCancelButtonText("取消");
    
    ok = inputDialog.exec();
    QString password = inputDialog.textValue();
    
    if (ok) {
        if (password == adminPassword) {
            setAdminMode(true);
            QMessageBox::information(this, "登录成功", "已进入管理员模式");
        } else {
            QMessageBox::warning(this, "登录失败", "密码错误，请重试");
        }
    }
}

void SystemWindow::setAdminMode(bool isAdmin)
{
    isAdminMode = isAdmin;
    updateAdminButtonsState();
}

void SystemWindow::updateAdminButtonsState()
{
    // 值周管理界面：恢复排班按钮、查看历史记录按钮
    ui->alterButton->setEnabled(isAdminMode);
    ui->clearButton->setEnabled(isAdminMode);
    
    // 队员管理界面：三个次数统计 SpinBox
    // 设置启用/禁用状态
    ui->total_times_spinBox->setEnabled(isAdminMode);
    ui->njh_total_times_spinBox->setEnabled(isAdminMode);
    ui->dxy_total_times_spinBox->setEnabled(isAdminMode);
    
    // 设置只读状态（管理员模式下可编辑）
    ui->total_times_spinBox->setReadOnly(!isAdminMode);
    ui->njh_total_times_spinBox->setReadOnly(!isAdminMode);
    ui->dxy_total_times_spinBox->setReadOnly(!isAdminMode);
    
    // 设置按钮符号（管理员模式下显示上下箭头）
    if (isAdminMode) {
        ui->total_times_spinBox->setButtonSymbols(QAbstractSpinBox::UpDownArrows);
        ui->njh_total_times_spinBox->setButtonSymbols(QAbstractSpinBox::UpDownArrows);
        ui->dxy_total_times_spinBox->setButtonSymbols(QAbstractSpinBox::UpDownArrows);
        
        // 设置焦点策略（管理员模式下可以获得焦点）
        ui->total_times_spinBox->setFocusPolicy(Qt::StrongFocus);
        ui->njh_total_times_spinBox->setFocusPolicy(Qt::StrongFocus);
        ui->dxy_total_times_spinBox->setFocusPolicy(Qt::StrongFocus);
    } else {
        ui->total_times_spinBox->setButtonSymbols(QAbstractSpinBox::NoButtons);
        ui->njh_total_times_spinBox->setButtonSymbols(QAbstractSpinBox::NoButtons);
        ui->dxy_total_times_spinBox->setButtonSymbols(QAbstractSpinBox::NoButtons);
        
        // 普通模式下不可获得焦点
        ui->total_times_spinBox->setFocusPolicy(Qt::NoFocus);
        ui->njh_total_times_spinBox->setFocusPolicy(Qt::NoFocus);
        ui->dxy_total_times_spinBox->setFocusPolicy(Qt::NoFocus);
    }
}

// 事件过滤器，用于检测使用说明文本框的双击事件
bool SystemWindow::eventFilter(QObject *obj, QEvent *event)
{
    // 检查是否是使用说明文本框或其视口的双击事件
    if ((obj == ui->instructionText || obj == ui->instructionText->viewport()) && 
        event->type() == QEvent::MouseButtonDblClick) {
        
        // 检查当前是否在使用说明标签页
        if (ui->worksheet_tabWidget->currentIndex() == 2) { // 使用说明是第3个标签页，索引为2
            onInstructionTabDoubleClicked();
            return true; // 事件已处理
        }
    }
    
    // 其他事件交给父类处理
    return QMainWindow::eventFilter(obj, event);
}

// 使用说明标签页双击事件处理
void SystemWindow::onInstructionTabDoubleClicked()
{
    instructionTabClickCount++;
    
    // 启动或重启计时器（1000ms内的点击才算连续）
    clickTimer->start(1000);
    
    // 连续双击5次触发彩蛋
    if (instructionTabClickCount >= 5) {
        instructionTabClickCount = 0;
        clickTimer->stop();
        showEasterEgg();
    }
}

// 彩蛋窗口
void SystemWindow::showEasterEgg()
{
    // 创建彩蛋对话框
    QDialog *easterEggDialog = new QDialog(this);
    easterEggDialog->setWindowTitle("🎉 彩蛋");
    easterEggDialog->setMinimumSize(600, 400);
    easterEggDialog->setModal(true);
    
    // 创建布局
    QVBoxLayout *layout = new QVBoxLayout(easterEggDialog);
    
    // 添加标题标签
    QLabel *titleLabel = new QLabel("🎊 恭喜你发现了隐藏彩蛋！ 🎊", easterEggDialog);
    titleLabel->setAlignment(Qt::AlignCenter);
    QFont titleFont = titleLabel->font();
    titleFont.setPointSize(16);
    titleFont.setBold(true);
    titleLabel->setFont(titleFont);
    layout->addWidget(titleLabel);
    
    // 添加说明标签
    QString infoText = isAdminMode ? "你可以在下方编辑彩蛋内容：" : "彩蛋内容（只读模式，需要管理员权限才能编辑）：";
    QLabel *infoLabel = new QLabel(infoText, easterEggDialog);
    infoLabel->setAlignment(Qt::AlignLeft);
    layout->addWidget(infoLabel);
    
    // 添加文本编辑框
    QTextEdit *textEdit = new QTextEdit(easterEggDialog);
    textEdit->setPlaceholderText("在这里输入你想要的彩蛋内容...\n\n可以写一些有趣的话、祝福语、或者任何你想说的话！");
    
    // 根据管理员模式设置是否可编辑
    textEdit->setReadOnly(!isAdminMode);
    
    // 设置默认彩蛋内容
    QString defaultEasterEggText = 
        "🎉 恭喜你发现了这个隐藏彩蛋！ 🎉\n\n"
        "═══════════════════════════════════════\n\n"
        "感谢你使用 WHUT国仪执勤管理系统！\n\n"
        "这个系统是为了让排班工作更加轻松高效而开发的。\n"
        "希望它能为你的工作带来便利！\n\n"
        "═══════════════════════════════════════\n\n"
        "💡 小提示：\n"
        "- 快捷键 Ctrl+Shift+A 可以进入管理员模式\n"
        "- 记得定期备份 data 文件夹哦\n"
        "- 有任何问题欢迎联系开发者\n\n"
        "═══════════════════════════════════════\n\n"
        "开发者的话：\n\n"
        "本来最初想在这个程序里设置点彩蛋，想了想，还是算了，\n"
        "毕竟也不一定会真正用这个东西排表，而且要是真用了，\n"
        "自己留个彩蛋还怪不好意思的。\n\n"
        "但是现在，既然你发现了这个彩蛋，那就说明你真的在\n"
        "认真使用这个系统！这让我感到非常开心和欣慰！\n\n"
        "感谢你的使用！\n\n"
        "═══════════════════════════════════════\n\n"
        "版本：v1.2.2\n"
        "开发者：万明\n"
        "日期：2026年2月\n\n"
        "🌟 祝你工作顺利，生活愉快！ 🌟";
    
    textEdit->setPlainText(defaultEasterEggText);
    layout->addWidget(textEdit);
    
    // 添加按钮
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    
    // 只有管理员模式才创建和显示保存按钮
    if (isAdminMode) {
        QPushButton *saveButton = new QPushButton("保存修改", easterEggDialog);
        buttonLayout->addWidget(saveButton);
        
        // 连接保存按钮信号
        connect(saveButton, &QPushButton::clicked, [textEdit, easterEggDialog, this]() {
            // 保存彩蛋内容到加密文件
            QString easterEggContent = textEdit->toPlainText();
            QString encryptedFilename = "./data/easter_egg.dat";
            
            // 确保data目录存在
            QFileInfo fileInfo(encryptedFilename);
            QDir dir = fileInfo.absoluteDir();
            if (!dir.exists()) {
                dir.mkpath(".");
            }
            
            // 使用加密保存
            if (EncryptedFileManager::saveEasterEgg(easterEggContent, encryptedFilename)) {
                QMessageBox::information(easterEggDialog, "保存成功", "彩蛋内容已加密保存！");
            } else {
                QMessageBox::warning(easterEggDialog, "保存失败", "无法保存彩蛋内容到文件。");
            }
        });
    }
    
    buttonLayout->addStretch();
    
    QPushButton *closeButton = new QPushButton("关闭", easterEggDialog);
    buttonLayout->addWidget(closeButton);
    
    layout->addLayout(buttonLayout);
    
    // 连接关闭按钮信号
    connect(closeButton, &QPushButton::clicked, easterEggDialog, &QDialog::accept);
    
    // 尝试从加密文件加载彩蛋内容
    QString encryptedFilename = "./data/easter_egg.dat";
    QFile file(encryptedFilename);
    if (file.exists()) {
        QString savedContent = EncryptedFileManager::loadEasterEgg(encryptedFilename);
        if (!savedContent.isEmpty()) {
            textEdit->setPlainText(savedContent);
        }
    }
    
    // 显示对话框
    easterEggDialog->exec();
    delete easterEggDialog;
}
