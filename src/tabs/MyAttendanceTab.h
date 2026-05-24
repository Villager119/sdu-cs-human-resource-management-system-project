#ifndef MYATTENDANCETAB_H
#define MYATTENDANCETAB_H

#include <QWidget>
#include <QTableView>
#include <QSqlRelationalTableModel>
#include <QComboBox>
#include <QLineEdit>
#include <QPushButton>
#include <QDateEdit>
#include <QTimeEdit>
#include <QTextEdit>
#include <QLabel>
#include <QTabWidget>
#include <QTimer>
#include <functional>

class MyAttendanceTab : public QWidget
{
    Q_OBJECT
public:
    MyAttendanceTab(int empId, const QString &role,
                    std::function<void(const QString&, const QString&)> logFn,
                    std::function<void(int, const QString&, const QString&)> notifyFn,
                    QWidget *parent = nullptr);
    void refresh();

private slots:
    void updateClock();
    void clockIn();
    void clockOut();
    void submitLeave();
    void submitMakeup();

private:
    int m_empId;
    QString m_role;
    std::function<void(const QString&, const QString&)> m_log;
    std::function<void(int, const QString&, const QString&)> m_notify;

    // Daily Clock page
    QLabel *m_lblDate;
    QLabel *m_lblTime;
    QLabel *m_lblStatus;
    QPushButton *m_btnClockIn;
    QPushButton *m_btnClockOut;
    QTableView *m_attTable;
    QSqlRelationalTableModel *m_attModel;
    QTimer *m_clockTimer;

    // Leave request components
    QDateEdit *m_leaveStart;
    QDateEdit *m_leaveEnd;
    QTextEdit *m_leaveReason;
    QTableView *m_leaveTable;
    QSqlRelationalTableModel *m_leaveModel;

    // Makeup request components
    QDateEdit *m_makeupDate;
    QComboBox *m_makeupType;
    QTimeEdit *m_makeupTime;
    QTextEdit *m_makeupReason;
    QTableView *m_makeupTable;
    QSqlRelationalTableModel *m_makeupModel;

    QWidget *createDailyClockWidget();
    QWidget *createAppCenterWidget();
};

#endif // MYATTENDANCETAB_H
