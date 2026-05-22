#ifndef ATTENDTAXTAB_H
#define ATTENDTAXTAB_H

#include <QWidget>
#include <QTableView>
#include <QSqlRelationalTableModel>
#include <QSqlRelation>
#include <QComboBox>
#include <QLineEdit>
#include <QPushButton>
#include <QDateEdit>
#include <QTimeEdit>
#include <QCheckBox>
#include <functional>

class AttendTaxTab : public QWidget
{
    Q_OBJECT
public:
    AttendTaxTab(int empId, const QString &role,
                 std::function<void(const QString&, const QString&)> logFn,
                 std::function<void(int, const QString&, const QString&)> notifyFn,
                 QWidget *parent = nullptr);
    void refresh();

private slots:
    // 打卡
    void clockIn();
    void clockOut();
    // 补卡
    void submitMakeup();
    void approveMakeup();
    void rejectMakeup();
    // 切换视图
    void switchView(int idx);
    // 过滤考勤
    void filterAttendance();

private:
    void initTables();
    QWidget *createAttendancePanel();
    QWidget *createMakeupPanel();

    int m_empId;
    QString m_role;
    std::function<void(const QString&, const QString&)> m_log;
    std::function<void(int, const QString&, const QString&)> m_notify;

    // 考勤
    QTableView *m_attTable;
    QSqlRelationalTableModel *m_attModel;
    QCheckBox *m_chkDateFilter;
    QDateEdit *m_attDateFilter;
    QLineEdit *m_attNameFilter;
    QComboBox *m_attStatusCombo;
    QPushButton *m_btnSearchAtt;
    QPushButton *m_btnResetAtt;

    // 补卡
    QTableView *m_makeupTable;
    QSqlRelationalTableModel *m_makeupModel;
    QDateEdit *m_makeupDate;
    QComboBox *m_makeupType;
    QTimeEdit *m_makeupTime;
    QLineEdit *m_makeupReason;
    QPushButton *m_btnApproveMakeup, *m_btnRejectMakeup;
    // 视图
    QTabWidget *m_viewTabs;
};

#endif

