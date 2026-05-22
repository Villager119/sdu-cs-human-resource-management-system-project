#ifndef LEAVETAB_H
#define LEAVETAB_H

#include <QWidget>
#include <QSqlRelationalTableModel>
#include <QTableView>
#include <QDateEdit>
#include <QLineEdit>
#include <QPushButton>
#include <functional>

class LeaveTab : public QWidget
{
    Q_OBJECT
public:
    LeaveTab(int empId, const QString &role,
             std::function<void(const QString&, const QString&)> logFn,
             std::function<void(int, const QString&, const QString&)> notifyFn,
             QWidget *parent = nullptr);
    void refresh();

private slots:
    void applyLeave();
    void approve();
    void reject();

private:
    QSqlRelationalTableModel *m_model;
    QTableView *m_table;
    QDateEdit *m_start, *m_end;
    QLineEdit *m_reason;
    QPushButton *m_btnApprove, *m_btnReject;
    int m_empId;
    QString m_role;
    std::function<void(const QString&, const QString&)> m_log;
    std::function<void(int, const QString&, const QString&)> m_notify;
};

#endif
