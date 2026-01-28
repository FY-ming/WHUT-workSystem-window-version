// historyDialog.h头文件
// 功能说明：历史记录查看和回退窗口

#pragma once
#include <QDialog>
#include <QListWidget>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QTextEdit>
#include <QMessageBox>
#include <QAbstractItemView>
#include "scheduleHistory.h"
#include "Flag_group.h"
#include "dataFunction.h"

class HistoryDialog : public QDialog
{
    Q_OBJECT

public:
    explicit HistoryDialog(ScheduleHistoryManager* historyManager, 
                          Flag_group* currentFlagGroup,
                          QWidget *parent = nullptr);
    
    // 获取用户选择要回退的历史记录索引（-1表示未选择）
    int getSelectedHistoryIndex() const { return selectedIndex; }

signals:
    void restoreHistoryRequested(int historyIndex); // 请求回退到指定历史记录

private slots:
    void onHistoryItemClicked(QListWidgetItem* item);
    void onRestoreButtonClicked();
    void onDeleteButtonClicked();
    void onCloseButtonClicked();

private:
    void updateHistoryList();
    void showHistoryDetails(int index);
    
    ScheduleHistoryManager* historyManager;
    Flag_group* currentFlagGroup;
    QListWidget* historyListWidget;
    QTextEdit* detailsTextEdit;
    QLabel* detailsLabel;
    QPushButton* restoreButton;
    QPushButton* deleteButton;
    QPushButton* closeButton;
    int selectedIndex;
};
