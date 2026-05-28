#ifndef ATTENDANCESERVICE_H
#define ATTENDANCESERVICE_H

#include <QDate>
#include <QSqlDatabase>
#include <QString>
#include <QTime>

class AttendanceService
{
public:
    struct TodayStatus {
        QString shiftStart = "09:00";
        QString shiftEnd = "18:00";
        bool hasRecord = false;
        QString clockIn;
        QString clockOut;
        QString status;
    };

    struct Result {
        bool success = false;
        QString message;
        QString logAction;
        QString logDetails;
        QString notificationTitle;
        QString notificationContent;
        QString errorMessage;
    };

    explicit AttendanceService(const QSqlDatabase &db = QSqlDatabase::database());

    TodayStatus todayStatus(int employeeId) const;
    Result clockIn(int employeeId, const QDate &date, const QTime &time);
    Result clockOut(int employeeId, const QDate &date, const QTime &time);
    Result submitLeaveRequest(int employeeId, const QDate &startDate, const QDate &endDate, const QString &reason);
    Result submitMakeupRequest(int employeeId, const QDate &date, const QString &type,
                               const QTime &time, const QString &reason);

private:
    struct ShiftTimes {
        QString start = "09:00:00";
        QString end = "18:00:00";
    };

    Result fail(const QString &message) const;
    ShiftTimes shiftTimes() const;
    bool hasLeaveOverlap(int employeeId, const QDate &startDate, const QDate &endDate) const;

    QSqlDatabase m_db;
};

#endif
