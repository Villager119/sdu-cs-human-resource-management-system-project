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
#include <functional>

class AttendTaxTab : public QWidget
{
    Q_OBJECT
public:
    AttendTaxTab(int empId, const QString &role,
                 std::function<void(const QString&, const QString&)> logFn,
                 QWidget *parent = nullptr);

private slots:
    // 打卡
    void clockIn();
    void clockOut();
    // 补卡
    void submitMakeup();
    void approveMakeup();
    void rejectMakeup();
    // 社保配置
    void saveTaxConfig();
    // 切换视图
    void switchView(int idx);

private:
    void initTables();
    QWidget *createAttendancePanel();
    QWidget *createMakeupPanel();
    QWidget *createTaxConfigPanel();

    int m_empId;
    QString m_role;
    std::function<void(const QString&, const QString&)> m_log;

    // 考勤
    QTableView *m_attTable;
    QSqlRelationalTableModel *m_attModel;
    // 补卡
    QTableView *m_makeupTable;
    QSqlRelationalTableModel *m_makeupModel;
    QDateEdit *m_makeupDate;
    QComboBox *m_makeupType;
    QTimeEdit *m_makeupTime;
    QLineEdit *m_makeupReason;
    QPushButton *m_btnApproveMakeup, *m_btnRejectMakeup;
    // 排班
    QTableView *m_shiftTable;
    QSqlTableModel *m_shiftModel;
    // 社保配置
    QTableView *m_taxTable;
    QSqlTableModel *m_taxModel;
    QPushButton *m_btnSaveTax;
    // 视图
    QTabWidget *m_viewTabs;
};

#endif
