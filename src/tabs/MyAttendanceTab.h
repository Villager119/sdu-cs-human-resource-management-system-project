#ifndef MYATTENDANCETAB_H
#define MYATTENDANCETAB_H

#include <QWidget>
#include <functional>

class DailyClockWidget;
class AppCenterWidget;

class MyAttendanceTab : public QWidget
{
    Q_OBJECT
public:
    MyAttendanceTab(int empId, const QString &role,
                    std::function<void(const QString&, const QString&)> logFn,
                    std::function<void(int, const QString&, const QString&)> notifyFn,
                    QWidget *parent = nullptr);
    void refresh();

private:
    int m_empId;
    QString m_role;
    std::function<void(const QString&, const QString&)> m_log;
    std::function<void(int, const QString&, const QString&)> m_notify;

    DailyClockWidget *m_clockWidget;
    AppCenterWidget *m_appWidget;
};

#endif // MYATTENDANCETAB_H
