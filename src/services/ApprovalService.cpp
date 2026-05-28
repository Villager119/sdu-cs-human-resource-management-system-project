#include "ApprovalService.h"

#include "../core/Constants.h"

#include <QSqlError>
#include <QSqlQuery>

ApprovalService::ApprovalService(const QSqlDatabase &db)
    : m_db(db)
{
}

ApprovalService::Result ApprovalService::reviewLeaveRequest(int requestId, bool approved)
{
    const int employeeId = employeeIdForLeaveRequest(requestId);
    if (employeeId <= 0) {
        return fail("未找到请假申请对应的员工");
    }

    QString errorText;
    if (!updateLeaveStatus(requestId, approved, &errorText)) {
        return fail("更新请假申请状态失败: " + errorText);
    }

    Result result;
    result.success = true;
    result.employeeId = employeeId;
    result.logAction = approved ? "同意请假" : "拒绝请假";
    result.logDetails = "单号: " + QString::number(requestId);
    result.notificationTitle = approved ? "请假已批准" : "请假已拒绝";
    result.notificationContent = approved ? "你的请假申请已通过审批" : "你的请假申请被拒绝";
    return result;
}

ApprovalService::Result ApprovalService::reviewMakeupRequest(int makeupId, bool approved)
{
    const int employeeId = employeeIdForMakeupRequest(makeupId);
    if (employeeId <= 0) {
        return fail("未找到补卡申请对应的员工");
    }

    QString errorText;
    if (!m_db.transaction()) {
        return fail("启动补卡审批事务失败: " + m_db.lastError().text());
    }

    if (!updateMakeupStatus(makeupId, approved, &errorText)) {
        m_db.rollback();
        return fail("更新补卡申请状态失败: " + errorText);
    }

    if (approved && !applyMakeupToAttendance(makeupId, employeeId, &errorText)) {
        m_db.rollback();
        return fail("更新考勤数据失败: " + errorText);
    }

    if (!m_db.commit()) {
        return fail("提交补卡审批事务失败: " + m_db.lastError().text());
    }

    Result result;
    result.success = true;
    result.employeeId = employeeId;
    result.logAction = approved ? "同意补卡" : "拒绝补卡";
    result.logDetails = "单号: " + QString::number(makeupId);
    result.notificationTitle = approved ? "补卡申请已批准" : "补卡申请已拒绝";
    result.notificationContent = approved ? "你的补卡申请已通过审批" : "你的补卡申请被拒绝";
    return result;
}

ApprovalService::Result ApprovalService::fail(const QString &message) const
{
    Result result;
    result.errorMessage = message;
    return result;
}

int ApprovalService::employeeIdForLeaveRequest(int requestId) const
{
    QSqlQuery query(m_db);
    query.prepare("SELECT emp_id FROM leave_requests WHERE request_id=?");
    query.addBindValue(requestId);

    int employeeId = 0;
    if (query.exec() && query.next()) {
        employeeId = query.value(0).toInt();
    }
    query.finish();
    return employeeId;
}

int ApprovalService::employeeIdForMakeupRequest(int makeupId) const
{
    QSqlQuery query(m_db);
    query.prepare("SELECT emp_id FROM makeup_requests WHERE makeup_id=?");
    query.addBindValue(makeupId);

    int employeeId = 0;
    if (query.exec() && query.next()) {
        employeeId = query.value(0).toInt();
    }
    query.finish();
    return employeeId;
}

bool ApprovalService::updateLeaveStatus(int requestId, bool approved, QString *errorText)
{
    QSqlQuery query(m_db);
    query.prepare("UPDATE leave_requests SET status=? WHERE request_id=?");
    query.addBindValue(approved ? HR::LeaveStatus::APPROVED : HR::LeaveStatus::REJECTED);
    query.addBindValue(requestId);

    const bool ok = query.exec();
    if (!ok && errorText) {
        *errorText = query.lastError().text();
    }
    query.finish();
    return ok;
}

bool ApprovalService::updateMakeupStatus(int makeupId, bool approved, QString *errorText)
{
    QSqlQuery query(m_db);
    query.prepare("UPDATE makeup_requests SET status=? WHERE makeup_id=?");
    query.addBindValue(approved ? "已同意" : "已拒绝");
    query.addBindValue(makeupId);

    const bool ok = query.exec();
    if (!ok && errorText) {
        *errorText = query.lastError().text();
    }
    query.finish();
    return ok;
}

bool ApprovalService::applyMakeupToAttendance(int makeupId, int employeeId, QString *errorText)
{
    QSqlQuery query(m_db);
    query.prepare("SELECT att_date, request_type, request_time FROM makeup_requests WHERE makeup_id=?");
    query.addBindValue(makeupId);

    if (!query.exec() || !query.next()) {
        if (errorText) {
            *errorText = query.lastError().isValid() ? query.lastError().text() : "补卡申请不存在";
        }
        query.finish();
        return false;
    }

    const QString date = query.value(0).toString();
    const QString typeRaw = query.value(1).toString();
    const QString time = query.value(2).toString();
    query.finish();

    int attendanceId = -1;
    if (attendanceRecordFor(employeeId, date, &attendanceId)) {
        if (!updateAttendanceTime(attendanceId, typeRaw, time, errorText)) {
            return false;
        }
        return refreshAttendanceStatus(attendanceId, errorText);
    }

    return createAttendanceRecord(employeeId, date, typeRaw, time, errorText);
}

bool ApprovalService::attendanceRecordFor(int employeeId, const QString &date, int *attendanceId) const
{
    QSqlQuery query(m_db);
    query.prepare("SELECT att_id FROM attendances WHERE emp_id=? AND att_date=?");
    query.addBindValue(employeeId);
    query.addBindValue(date);

    bool exists = false;
    if (query.exec() && query.next()) {
        if (attendanceId) {
            *attendanceId = query.value(0).toInt();
        }
        exists = true;
    }
    query.finish();
    return exists;
}

bool ApprovalService::updateAttendanceTime(int attendanceId, const QString &typeRaw, const QString &time, QString *errorText)
{
    QSqlQuery query(m_db);
    if (typeRaw == "clock_in") {
        query.prepare("UPDATE attendances SET clock_in=? WHERE att_id=?");
    } else {
        query.prepare("UPDATE attendances SET clock_out=? WHERE att_id=?");
    }
    query.addBindValue(time);
    query.addBindValue(attendanceId);

    const bool ok = query.exec();
    if (!ok && errorText) {
        *errorText = query.lastError().text();
    }
    query.finish();
    return ok;
}

bool ApprovalService::createAttendanceRecord(int employeeId, const QString &date, const QString &typeRaw,
                                             const QString &time, QString *errorText)
{
    QSqlQuery query(m_db);
    if (typeRaw == "clock_in") {
        query.prepare("INSERT INTO attendances(emp_id, att_date, clock_in, status, remark) "
                      "VALUES(?,?,?,'正常','补卡录入')");
    } else {
        query.prepare("INSERT INTO attendances(emp_id, att_date, clock_out, status, remark) "
                      "VALUES(?,?,?,'正常','补卡录入')");
    }
    query.addBindValue(employeeId);
    query.addBindValue(date);
    query.addBindValue(time);

    const bool ok = query.exec();
    if (!ok && errorText) {
        *errorText = query.lastError().text();
    }
    query.finish();
    return ok;
}

bool ApprovalService::refreshAttendanceStatus(int attendanceId, QString *errorText)
{
    QString start = "09:00:00";
    QString end = "18:00:00";
    QSqlQuery shiftQuery(m_db);
    if (shiftQuery.exec("SELECT start_time, end_time FROM shifts WHERE shift_id=1") && shiftQuery.next()) {
        start = shiftQuery.value(0).toString();
        end = shiftQuery.value(1).toString();
    }
    shiftQuery.finish();

    QSqlQuery readQuery(m_db);
    readQuery.prepare("SELECT clock_in, clock_out FROM attendances WHERE att_id=?");
    readQuery.addBindValue(attendanceId);
    if (!readQuery.exec() || !readQuery.next()) {
        if (errorText) {
            *errorText = readQuery.lastError().isValid() ? readQuery.lastError().text() : "考勤记录不存在";
        }
        readQuery.finish();
        return false;
    }
    const QString clockIn = readQuery.value(0).toString();
    const QString clockOut = readQuery.value(1).toString();
    readQuery.finish();

    QString status = "正常";
    if (!clockIn.isEmpty() && clockIn > start) status = "迟到";
    else if (!clockOut.isEmpty() && clockOut < end) status = "早退";

    QSqlQuery updateQuery(m_db);
    updateQuery.prepare("UPDATE attendances SET status=? WHERE att_id=?");
    updateQuery.addBindValue(status);
    updateQuery.addBindValue(attendanceId);

    const bool ok = updateQuery.exec();
    if (!ok && errorText) {
        *errorText = updateQuery.lastError().text();
    }
    updateQuery.finish();
    return ok;
}
