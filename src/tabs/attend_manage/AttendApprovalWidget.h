#ifndef ATTENDAPPROVALWIDGET_H
#define ATTENDAPPROVALWIDGET_H

#include <QWidget>
#include <QTableView>
#include <QSqlRelationalTableModel>
#include <QTabWidget>
#include <QPushButton>

class AttendApprovalWidget : public QWidget
{
    Q_OBJECT
public:
    explicit AttendApprovalWidget(int empId, const QString &role, QWidget *parent = nullptr);
    void refresh();

signals:
    void logRequested(const QString &action, const QString &details);
    void notificationRequested(int empId, const QString &title, const QString &content);

private slots:
    void openLeaveApproval();
    void openMakeupApproval();
    void approveSelectedLeaves();
    void rejectSelectedLeaves();
    void approveSelectedMakeups();
    void rejectSelectedMakeups();

private:
    void updateLeaveApprovalButton();
    void updateMakeupApprovalButton();
    void reviewSelectedLeaves(bool approved);
    void reviewSelectedMakeups(bool approved);

    QWidget *createLeaveApprovalPage();
    QWidget *createMakeupApprovalPage();

    int m_empId;
    QString m_role;

    QTabWidget *m_approvalTabs = nullptr;
    QTableView *m_leaveTable = nullptr;
    QSqlRelationalTableModel *m_leaveModel = nullptr;
    QTableView *m_makeupTable = nullptr;
    QSqlRelationalTableModel *m_makeupModel = nullptr;

    QPushButton *m_btnApproveLeave = nullptr;
    QPushButton *m_btnApproveLeaves = nullptr;
    QPushButton *m_btnRejectLeaves = nullptr;
    QPushButton *m_btnApproveMakeup = nullptr;
    QPushButton *m_btnApproveMakeups = nullptr;
    QPushButton *m_btnRejectMakeups = nullptr;
};

#endif // ATTENDAPPROVALWIDGET_H
