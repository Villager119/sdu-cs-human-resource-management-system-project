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

private:
    int m_empId;
    QString m_role;

    QTabWidget *m_approvalTabs;
    QTableView *m_leaveTable;
    QSqlRelationalTableModel *m_leaveModel;
    QTableView *m_makeupTable;
    QSqlRelationalTableModel *m_makeupModel;

    QPushButton *m_btnApproveLeave;
    QPushButton *m_btnApproveMakeup;
};

#endif // ATTENDAPPROVALWIDGET_H
