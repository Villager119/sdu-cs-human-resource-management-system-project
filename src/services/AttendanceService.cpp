#include "AttendanceService.h"

#include "../core/Constants.h"

#include <QSqlError>
#include <QSqlQuery>

AttendanceService::AttendanceService(const QSqlDatabase &db)
    : m_db(db)
{
}

AttendanceService::TodayStatus AttendanceService::todayStatus(int employeeId) const
{
    TodayStatus result;
    const ShiftTimes shift = shiftTimes();
    result.shiftStart = shift.start.left(5);
    result.shiftEnd = shift.end.left(5);

    QSqlQuery query(m_db);
    query.prepare("SELECT clock_in, clock_out, status FROM attendances WHERE emp_id = ? AND att_date = ?");
    query.addBindValue(employeeId);
    query.addBindValue(QDate::currentDate().toString("yyyy-MM-dd"));
    if (query.exec() && query.next()) {
        result.clockIn = query.value(0).toString();
        result.clockOut = query.value(1).toString();
        result.status = query.value(2).toString();
        result.hasRecord = true;
    }
    query.finish();
    return result;
}

AttendanceService::Result AttendanceService::clockIn(int employeeId, const QDate &date, const QTime &time)
{
    const QString dateText = date.toString("yyyy-MM-dd");
    const QString timeText = time.toString("HH:mm:ss");

    QSqlQuery existing(m_db);
    existing.prepare("SELECT att_id FROM attendances WHERE emp_id=? AND att_date=?");
    existing.addBindValue(employeeId);
    existing.addBindValue(dateText);
    const bool alreadyClocked = existing.exec() && existing.next();
    existing.finish();
    if (alreadyClocked) {
        return fail("今天已打卡，请勿重复");
    }

    const QString status = (timeText > shiftTimes().start) ? "迟到" : "正常";

    QSqlQuery query(m_db);
    query.prepare("INSERT INTO attendances(emp_id,att_date,clock_in,status) VALUES(?,?,?,?)");
    query.addBindValue(employeeId);
    query.addBindValue(dateText);
    query.addBindValue(timeText);
    query.addBindValue(status);

    const bool ok = query.exec();
    const QString errorText = query.lastError().text();
    query.finish();
    if (!ok) {
        return fail(errorText);
    }

    Result result;
    result.success = true;
    result.message = QString("上班打卡成功 (%1)").arg(timeText);
    result.logAction = "上班打卡";
    result.logDetails = dateText;
    return result;
}

AttendanceService::Result AttendanceService::clockOut(int employeeId, const QDate &date, const QTime &time)
{
    const QString dateText = date.toString("yyyy-MM-dd");
    const QString timeText = time.toString("HH:mm:ss");

    QSqlQuery read(m_db);
    read.prepare("SELECT att_id, status FROM attendances WHERE emp_id=? AND att_date=?");
    read.addBindValue(employeeId);
    read.addBindValue(dateText);

    QString currentStatus;
    const bool hasRecord = read.exec() && read.next();
    if (hasRecord) {
        currentStatus = read.value(1).toString();
    }
    read.finish();
    if (!hasRecord) {
        return fail("请先打上班卡");
    }

    QString status = (timeText < shiftTimes().end) ? "早退" : "正常";
    if (currentStatus == "迟到") {
        status = "迟到";
    }

    QSqlQuery query(m_db);
    query.prepare("UPDATE attendances SET clock_out=?, status=? WHERE emp_id=? AND att_date=?");
    query.addBindValue(timeText);
    query.addBindValue(status);
    query.addBindValue(employeeId);
    query.addBindValue(dateText);

    const bool ok = query.exec();
    const QString errorText = query.lastError().text();
    query.finish();
    if (!ok) {
        return fail(errorText);
    }

    Result result;
    result.success = true;
    result.message = QString("下班打卡成功 (%1) [%2]").arg(timeText, status);
    result.logAction = "下班打卡";
    result.logDetails = dateText;
    return result;
}

AttendanceService::Result AttendanceService::submitLeaveRequest(int employeeId, const QDate &startDate,
                                                                const QDate &endDate, const QString &reason)
{
    if (startDate > endDate) return fail("结束日期不能早于开始日期");
    if (reason.isEmpty()) return fail("请假事由不能为空");
    if (reason.length() < 5) return fail("请假事由字数不能少于5个字");
    if (hasLeaveOverlap(employeeId, startDate, endDate)) {
        return fail("在该日期段内已有尚未拒绝的请假申请");
    }

    QSqlQuery query(m_db);
    query.prepare("INSERT INTO leave_requests(emp_id,start_date,end_date,reason,status) VALUES(?,?,?,?,?)");
    query.addBindValue(employeeId);
    query.addBindValue(startDate.toString("yyyy-MM-dd"));
    query.addBindValue(endDate.toString("yyyy-MM-dd"));
    query.addBindValue(reason);
    query.addBindValue(HR::LeaveStatus::PENDING);

    const bool ok = query.exec();
    const QString errorText = query.lastError().text();
    query.finish();
    if (!ok) return fail(errorText);

    Result result;
    result.success = true;
    result.message = "请假申请已提交，等待审批";
    result.logAction = "提交请假申请";
    result.logDetails = startDate.toString("MM-dd") + " ~ " + endDate.toString("MM-dd");
    result.notificationTitle = "请假申请";
    result.notificationContent = QString("员工提交了请假申请 %1~%2")
        .arg(startDate.toString("MM-dd"), endDate.toString("MM-dd"));
    return result;
}

AttendanceService::Result AttendanceService::submitMakeupRequest(int employeeId, const QDate &date,
                                                                 const QString &type, const QTime &time,
                                                                 const QString &reason)
{
    if (reason.isEmpty()) return fail("补卡事由不能为空");
    if (reason.length() < 5) return fail("补卡事由字数不能少于5个字");

    QSqlQuery query(m_db);
    query.prepare("INSERT INTO makeup_requests(emp_id,att_date,request_type,request_time,reason,status) "
                  "VALUES(?,?,?,?,?,'待审批')");
    query.addBindValue(employeeId);
    query.addBindValue(date.toString("yyyy-MM-dd"));
    query.addBindValue(type);
    query.addBindValue(time.toString("HH:mm:ss"));
    query.addBindValue(reason);

    const bool ok = query.exec();
    const QString errorText = query.lastError().text();
    query.finish();
    if (!ok) return fail(errorText);

    Result result;
    result.success = true;
    result.message = "补卡申请已提交";
    result.logAction = "补卡申请";
    result.logDetails = date.toString("yyyy-MM-dd");
    result.notificationTitle = "补卡申请";
    result.notificationContent = QString("员工提交了补卡申请 日期：%1").arg(date.toString("yyyy-MM-dd"));
    return result;
}

AttendanceService::Result AttendanceService::fail(const QString &message) const
{
    Result result;
    result.errorMessage = message;
    return result;
}

AttendanceService::ShiftTimes AttendanceService::shiftTimes() const
{
    ShiftTimes result;
    QSqlQuery query(m_db);
    if (query.exec("SELECT start_time, end_time FROM shifts WHERE shift_id=1") && query.next()) {
        result.start = query.value(0).toString();
        result.end = query.value(1).toString();
    }
    query.finish();
    return result;
}

bool AttendanceService::hasLeaveOverlap(int employeeId, const QDate &startDate, const QDate &endDate) const
{
    QSqlQuery query(m_db);
    query.prepare("SELECT COUNT(*) FROM leave_requests "
                  "WHERE emp_id = ? AND status != ? AND NOT (end_date < ? OR start_date > ?)");
    query.addBindValue(employeeId);
    query.addBindValue(HR::LeaveStatus::REJECTED);
    query.addBindValue(startDate.toString("yyyy-MM-dd"));
    query.addBindValue(endDate.toString("yyyy-MM-dd"));

    const bool overlap = query.exec() && query.next() && query.value(0).toInt() > 0;
    query.finish();
    return overlap;
}
