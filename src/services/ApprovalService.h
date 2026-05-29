#ifndef APPROVALSERVICE_H
#define APPROVALSERVICE_H

#include <QDate>
#include <QSqlDatabase>
#include <QString>

class ApprovalService
{
public:
    struct Result {
        bool success = false;
        int employeeId = 0;
        QString logAction;
        QString logDetails;
        QString notificationTitle;
        QString notificationContent;
        QString errorMessage;
    };

    explicit ApprovalService(const QSqlDatabase &db = QSqlDatabase::database());

    Result reviewLeaveRequest(int requestId, bool approved, const QString &comment = QString(), int reviewerId = 0);
    Result reviewMakeupRequest(int makeupId, bool approved, const QString &comment = QString(), int reviewerId = 0);

private:
    Result fail(const QString &message) const;
    int employeeIdForLeaveRequest(int requestId) const;
    int employeeIdForMakeupRequest(int makeupId) const;
    bool updateLeaveStatus(int requestId, bool approved, const QString &comment, int reviewerId, QString *errorText);
    bool updateMakeupStatus(int makeupId, bool approved, const QString &comment, int reviewerId, QString *errorText);
    bool hasApprovedLeaveOverlap(int employeeId, const QDate &startDate, const QDate &endDate, int excludeRequestId, QString *errorText) const;
    bool applyMakeupToAttendance(int makeupId, int employeeId, QString *errorText);
    bool attendanceRecordFor(int employeeId, const QString &date, int *attendanceId) const;
    bool updateAttendanceTime(int attendanceId, const QString &typeRaw, const QString &time, QString *errorText);
    bool createAttendanceRecord(int employeeId, const QString &date, const QString &typeRaw,
                                const QString &time, QString *errorText);
    bool refreshAttendanceStatus(int attendanceId, QString *errorText);

    QSqlDatabase m_db;
};

#endif
