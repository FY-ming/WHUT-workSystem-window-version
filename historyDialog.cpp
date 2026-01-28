// historyDialog.cpp源文件
// 功能说明：历史记录查看和回退窗口的实现

#include "historyDialog.h"
#include <QDateTime>

HistoryDialog::HistoryDialog(ScheduleHistoryManager* historyManager, 
                            Flag_group* currentFlagGroup,
                            QWidget *parent)
    : QDialog(parent)
    , historyManager(historyManager)
    , currentFlagGroup(currentFlagGroup)
    , selectedIndex(-1)
{
    setWindowTitle("排班历史记录");
    setMinimumSize(800, 600);
    
    // 创建布局
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    
    // 历史记录列表
    QLabel* listLabel = new QLabel("历史记录列表（点击查看详情）：", this);
    mainLayout->addWidget(listLabel);
    
    historyListWidget = new QListWidget(this);
    historyListWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    mainLayout->addWidget(historyListWidget, 2);
    
    // 详情显示区域
    detailsLabel = new QLabel("详情：", this);
    mainLayout->addWidget(detailsLabel);
    
    detailsTextEdit = new QTextEdit(this);
    detailsTextEdit->setReadOnly(true);
    detailsTextEdit->setMaximumHeight(200);
    mainLayout->addWidget(detailsTextEdit);
    
    // 按钮区域
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    
    restoreButton = new QPushButton("回退到此记录", this);
    restoreButton->setEnabled(false);
    buttonLayout->addWidget(restoreButton);
    
    deleteButton = new QPushButton("删除此记录", this);
    deleteButton->setEnabled(false);
    buttonLayout->addWidget(deleteButton);
    
    buttonLayout->addStretch();
    
    closeButton = new QPushButton("关闭", this);
    buttonLayout->addWidget(closeButton);
    
    mainLayout->addLayout(buttonLayout);
    
    // 连接信号和槽
    connect(historyListWidget, &QListWidget::itemClicked, this, &HistoryDialog::onHistoryItemClicked);
    connect(restoreButton, &QPushButton::clicked, this, &HistoryDialog::onRestoreButtonClicked);
    connect(deleteButton, &QPushButton::clicked, this, &HistoryDialog::onDeleteButtonClicked);
    connect(closeButton, &QPushButton::clicked, this, &HistoryDialog::onCloseButtonClicked);
    
    // 更新历史记录列表
    updateHistoryList();
}

void HistoryDialog::updateHistoryList()
{
    historyListWidget->clear();
    
    const QList<ScheduleHistoryItem>& historyList = historyManager->getHistoryList();
    
    for (int i = historyList.size() - 1; i >= 0; --i) { // 从最新到最旧显示
        const ScheduleHistoryItem& item = historyList.at(i);
        QString timeStr = item.timestamp.toString("yyyy-MM-dd hh:mm:ss");
        QString displayText = QString("%1 - %2 (队员数: %3, 总排班次数: %4)")
                              .arg(timeStr)
                              .arg(item.mode)
                              .arg(item.totalMembers)
                              .arg(item.totalScheduleCount);
        
        QListWidgetItem* listItem = new QListWidgetItem(displayText, historyListWidget);
        listItem->setData(Qt::UserRole, i); // 存储实际索引（i就是历史记录列表中的实际索引）
    }
}

void HistoryDialog::onHistoryItemClicked(QListWidgetItem* item)
{
    if (!item) return;
    
    int index = item->data(Qt::UserRole).toInt();
    selectedIndex = index;
    
    showHistoryDetails(index);
    restoreButton->setEnabled(true);
    deleteButton->setEnabled(true);
}

void HistoryDialog::showHistoryDetails(int index)
{
    const ScheduleHistoryItem* item = historyManager->getHistory(index);
    if (!item) {
        detailsTextEdit->clear();
        return;
    }
    
    QString details;
    details += QString("制表时间：%1\n").arg(item->timestamp.toString("yyyy-MM-dd hh:mm:ss"));
    details += QString("排班模式：%1\n").arg(item->mode);
    details += QString("总队员数：%1\n").arg(item->totalMembers);
    details += QString("总排班次数：%1\n\n").arg(item->totalScheduleCount);
    details += "排班结果：\n";
    details += item->scheduleText;
    
    detailsTextEdit->setPlainText(details);
}

void HistoryDialog::onRestoreButtonClicked()
{
    if (selectedIndex < 0) return;
    
    QMessageBox::StandardButton reply = QMessageBox::question(
        this,
        "确认回退",
        QString("确定要回退到选中的历史记录吗？\n\n此操作会恢复该记录时的队员信息和排班状态，当前未保存的修改将会丢失。"),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No
    );
    
    if (reply == QMessageBox::Yes) {
        emit restoreHistoryRequested(selectedIndex);
        accept(); // 关闭对话框
    }
}

void HistoryDialog::onDeleteButtonClicked()
{
    if (selectedIndex < 0) return;
    
    QMessageBox::StandardButton reply = QMessageBox::question(
        this,
        "确认删除",
        QString("确定要删除选中的历史记录吗？\n\n此操作不可撤销。"),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No
    );
    
    if (reply == QMessageBox::Yes) {
        historyManager->removeHistory(selectedIndex);
        selectedIndex = -1;
        restoreButton->setEnabled(false);
        deleteButton->setEnabled(false);
        detailsTextEdit->clear();
        updateHistoryList();
    }
}

void HistoryDialog::onCloseButtonClicked()
{
    reject(); // 关闭对话框
}
