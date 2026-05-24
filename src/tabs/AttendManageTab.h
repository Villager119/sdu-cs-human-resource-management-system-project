#ifndef ATTENDMANAGETAB_H
#define ATTENDMANAGETAB_H

#include <QWidget>
#include <QTableView>
#include <QSqlRelationalTableModel>
#include <QComboBox>
#include <QLineEdit>
#include <QPushButton>
#include <QDateEdit>
#include <QLabel>
#include <QTabWidget>
#include <functional>

class AttendManageTab : public QWidget
{
    Q_OBJECT
public:
    AttendManageTab(int empId, const QString &role,
                    std::function<void(const QString&, const QString&)> logFn,
                    std::function<void(int, const QString&, const QString&)> notifyFn,
                    QWidget *parent = nullptr);
    void refresh();

private slots:
    void filterAttendance();
    void resetFilters();
    void exportCsv();
    void openLeaveApproval();
    void openMakeupApproval();

private:
    int m_empId;
    QString m_role;
    std::function<void(const QString&, const QString&)> m_log;
    std::function<void(int, const QString&, const QString&)> m_notify;

    // Board Tab
    QDateEdit *m_startDateFilter;
    QDateEdit *m_endDateFilter;
    QLineEdit *m_nameFilter;
    QComboBox *m_statusFilter;
    QTableView *m_attTable;
    QSqlRelationalTableModel *m_attModel;

    // Approval Tab
    QTabWidget *m_approvalTabs;
    QTableView *m_leaveTable;
    QSqlRelationalTableModel *m_leaveModel;
    QTableView *m_makeupTable;
    QSqlRelationalTableModel *m_makeupModel;

    QPushButton *m_btnApproveLeave;
    QPushButton *m_btnApproveMakeup;

    QWidget *createBoardWidget();
    QWidget *createApprovalCenterWidget();
};

#endif // ATTENDMANAGETAB_H
