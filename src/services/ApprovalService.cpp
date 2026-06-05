#include "ApprovalService.h"

#include "../core/Constants.h"
#include "AttendanceService.h"

#include <QSqlError>
#include <QSqlQuery>
#include <QMetaType>
#include <QVariant>

ApprovalService::ApprovalService(const QSqlDatabase &db)
    : m_db(db)
{
}

ApprovalService::Result ApprovalService::reviewLeaveRequest(int requestId, bool approved,
                                                             const QString &comment, int reviewerId)
{
    if (!approved && comment.trimmed().isEmpty()) {
        return fail("拒绝申请时必须填写审批意见，方便员工了解处理原因");
    }

    const int employeeId = employeeIdForLeaveRequest(requestId);
    if (employeeId <= 0) {
        return fail("未找到请假申请对应的员工");
    }

    if (!m_db.transaction()) {
        return fail("启动请假审批事务失败: " + m_db.lastError().text());
    }

    // 锁定员工行以序列化同一员工的审批操作
    QSqlQuery lockQuery(m_db);
    lockQuery.prepare("SELECT emp_id FROM employees WHERE emp_id=? FOR UPDATE");
    lockQuery.addBindValue(employeeId);
    if (!lockQuery.exec() || !lockQuery.next()) {
        const QString err = lockQuery.lastError().text();
        lockQuery.finish();
        m_db.rollback();
        return fail("锁定员工记录失败: " + err);
    }
    lockQuery.finish();

    QDate approvedStartDate;
    QDate approvedEndDate;
    if (approved) {
        QSqlQuery dq(m_db);
        dq.prepare("SELECT start_date, end_date FROM leave_requests WHERE request_id = ?");
        dq.addBindValue(requestId);
        if (!dq.exec() || !dq.next()) {
            dq.finish();
            m_db.rollback();
            return fail("未找到请假申请的日期信息");
        }
        QDate startDate = dq.value(0).toDate();
        QDate endDate = dq.value(1).toDate();
        dq.finish();

        if (!startDate.isValid() || !endDate.isValid() || startDate > endDate) {
            m_db.rollback();
            return fail("请假申请单的日期无效，无法同意此申请");
        }

        QString overlapErrorText;
        if (hasApprovedLeaveOverlap(employeeId, startDate, endDate, requestId, &overlapErrorText)) {
            m_db.rollback();
            if (!overlapErrorText.isEmpty()) {
                return fail("校验请假重叠失败: " + overlapErrorText);
            }
            return fail("该日期段内已有其他已同意的请假，无法同意此申请");
        }
        approvedStartDate = startDate;
        approvedEndDate = endDate;
    }

    QString errorText;
    if (!updateLeaveStatus(requestId, approved, comment.trimmed(), reviewerId, &errorText)) {
        m_db.rollback();
        return fail("更新请假申请状态失败: " + errorText);
    }

    if (approved && !AttendanceService(m_db).syncApprovedLeaveToAttendance(employeeId, approvedStartDate,
                                                                           approvedEndDate, &errorText)) {
        m_db.rollback();
        return fail("同步请假到考勤失败: " + errorText);
    }

    if (!m_db.commit()) {
        const QString commitErr = m_db.lastError().text();
        m_db.rollback();
        return fail("提交请假审批事务失败: " + commitErr);
    }

    Result result;
    result.success = true;
    result.employeeId = employeeId;
    result.logAction = approved ? "同意请假" : "拒绝请假";
    result.logDetails = "单号: " + QString::number(requestId);
    result.notificationTitle = approved ? "请假已批准" : "请假已拒绝";
    result.notificationContent = approved
        ? (comment.trimmed().isEmpty() ? "你的请假申请已通过审批" : "你的请假申请已通过审批，审批意见：" + comment.trimmed())
        : "你的请假申请被拒绝，原因：" + comment.trimmed();
    return result;
}

ApprovalService::Result ApprovalService::reviewMakeupRequest(int makeupId, bool approved,
                                                             const QString &comment, int reviewerId)
{
    if (!approved && comment.trimmed().isEmpty()) {
        return fail("拒绝申请时必须填写审批意见，方便员工了解处理原因");
    }

    const int employeeId = employeeIdForMakeupRequest(makeupId);
    if (employeeId <= 0) {
        return fail("未找到补卡申请对应的员工");
    }

    if (!m_db.transaction()) {
        return fail("启动补卡审批事务失败: " + m_db.lastError().text());
    }

    // 锁定员工行以序列化同一员工的审批操作
    QSqlQuery lockQuery(m_db);
    lockQuery.prepare("SELECT emp_id FROM employees WHERE emp_id=? FOR UPDATE");
    lockQuery.addBindValue(employeeId);
    if (!lockQuery.exec() || !lockQuery.next()) {
        const QString err = lockQuery.lastError().text();
        lockQuery.finish();
        m_db.rollback();
        return fail("锁定员工记录失败: " + err);
    }
    lockQuery.finish();

    if (approved) {
        // 读取当前补卡的日期和类型
        QSqlQuery rq(m_db);
        rq.prepare("SELECT att_date, request_type FROM makeup_requests WHERE makeup_id=?");
        rq.addBindValue(makeupId);
        if (!rq.exec() || !rq.next()) {
            rq.finish();
            m_db.rollback();
            return fail("未找到补卡申请的日期或类型信息");
        }
        QDate attDate = rq.value(0).toDate();
        QString requestType = rq.value(1).toString();
        rq.finish();

        if (!attDate.isValid() || (requestType != "clock_in" && requestType != "clock_out")) {
            m_db.rollback();
            return fail("补卡申请的日期或类型无效，无法同意此申请");
        }

        QString leaveErrorText;
        if (hasApprovedLeaveOnDate(employeeId, attDate, &leaveErrorText)) {
            m_db.rollback();
            if (!leaveErrorText.isEmpty()) {
                return fail("校验请假状态失败: " + leaveErrorText);
            }
            return fail("该员工当天已有已同意请假，无法再同意补卡申请");
        }

        // 检查是否已有相同日期和类型的已同意补卡记录
        QSqlQuery cq(m_db);
        cq.prepare("SELECT COUNT(*) FROM makeup_requests WHERE emp_id=? AND att_date=? AND request_type=? AND status=? AND makeup_id!=?");
        cq.addBindValue(employeeId);
        cq.addBindValue(attDate.toString("yyyy-MM-dd"));
        cq.addBindValue(requestType);
        cq.addBindValue(HR::LeaveStatus::APPROVED);
        cq.addBindValue(makeupId);
        if (!cq.exec()) {
            cq.finish();
            m_db.rollback();
            return fail("校验重复补卡审批失败: " + cq.lastError().text());
        }
        if (cq.next() && cq.value(0).toInt() > 0) {
            cq.finish();
            m_db.rollback();
            return fail("该员工当天已有同类型的补卡审批通过，无法重复同意");
        }
        cq.finish();
    }

    QString errorText;
    if (!updateMakeupStatus(makeupId, approved, comment.trimmed(), reviewerId, &errorText)) {
        m_db.rollback();
        return fail("更新补卡申请状态失败: " + errorText);
    }

    if (approved && !applyMakeupToAttendance(makeupId, employeeId, &errorText)) {
        m_db.rollback();
        return fail("更新考勤数据失败: " + errorText);
    }

    if (!m_db.commit()) {
        const QString commitErr = m_db.lastError().text();
        m_db.rollback();
        return fail("提交补卡审批事务失败: " + commitErr);
    }

    Result result;
    result.success = true;
    result.employeeId = employeeId;
    result.logAction = approved ? "同意补卡" : "拒绝补卡";
    result.logDetails = "单号: " + QString::number(makeupId);
    result.notificationTitle = approved ? "补卡申请已批准" : "补卡申请已拒绝";
    result.notificationContent = approved
        ? (comment.trimmed().isEmpty() ? "你的补卡申请已通过审批" : "你的补卡申请已通过审批，审批意见：" + comment.trimmed())
        : "你的补卡申请被拒绝，原因：" + comment.trimmed();
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

bool ApprovalService::updateLeaveStatus(int requestId, bool approved, const QString &comment,
                                        int reviewerId, QString *errorText)
{
    QSqlQuery query(m_db);
    query.prepare("UPDATE leave_requests SET status=?, reviewer_id=?, reviewed_at=NOW(), review_comment=? "
                  "WHERE request_id=? AND status=?");
    query.addBindValue(approved ? HR::LeaveStatus::APPROVED : HR::LeaveStatus::REJECTED);
    query.addBindValue(reviewerId > 0 ? QVariant(reviewerId) : QVariant(QMetaType(QMetaType::Int)));
    query.addBindValue(comment);
    query.addBindValue(requestId);
    query.addBindValue(HR::LeaveStatus::PENDING);

    const bool ok = query.exec();
    const int affectedRows = ok ? query.numRowsAffected() : -1;
    if (!ok && errorText) {
        *errorText = query.lastError().text();
    }
    if (ok && affectedRows == 0 && errorText) {
        *errorText = "申请不存在或已被处理，请刷新列表后重试";
    }
    query.finish();
    return ok && affectedRows != 0;
}

bool ApprovalService::updateMakeupStatus(int makeupId, bool approved, const QString &comment,
                                         int reviewerId, QString *errorText)
{
    QSqlQuery query(m_db);
    query.prepare("UPDATE makeup_requests SET status=?, reviewer_id=?, reviewed_at=NOW(), review_comment=? "
                  "WHERE makeup_id=? AND status=?");
    query.addBindValue(approved ? HR::LeaveStatus::APPROVED : HR::LeaveStatus::REJECTED);
    query.addBindValue(reviewerId > 0 ? QVariant(reviewerId) : QVariant(QMetaType(QMetaType::Int)));
    query.addBindValue(comment);
    query.addBindValue(makeupId);
    query.addBindValue(HR::LeaveStatus::PENDING);

    const bool ok = query.exec();
    const int affectedRows = ok ? query.numRowsAffected() : -1;
    if (!ok && errorText) {
        *errorText = query.lastError().text();
    }
    if (ok && affectedRows == 0 && errorText) {
        *errorText = "申请不存在或已被处理，请刷新列表后重试";
    }
    query.finish();
    return ok && affectedRows != 0;
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
        return AttendanceService(m_db).refreshAttendanceStatus(attendanceId, errorText);
    }

    if (!createAttendanceRecord(employeeId, date, typeRaw, time, errorText)) {
        return false;
    }

    int createdAttendanceId = -1;
    if (!attendanceRecordFor(employeeId, date, &createdAttendanceId)) {
        if (errorText) {
            *errorText = "补卡考勤记录创建后无法读取";
        }
        return false;
    }
    return AttendanceService(m_db).refreshAttendanceStatus(createdAttendanceId, errorText);
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
    QSqlQuery read(m_db);
    read.prepare("SELECT clock_in, clock_out FROM attendances WHERE att_id=?");
    read.addBindValue(attendanceId);
    if (!read.exec() || !read.next()) {
        if (errorText) {
            *errorText = read.lastError().isValid() ? read.lastError().text() : "考勤记录不存在";
        }
        read.finish();
        return false;
    }
    const QString existingTime = (typeRaw == "clock_in") ? read.value(0).toString() : read.value(1).toString();
    read.finish();

    if (!existingTime.isEmpty()) {
        if (errorText) {
            *errorText = (typeRaw == "clock_in")
                ? "当天已有上班打卡记录，无法用补卡覆盖"
                : "当天已有下班打卡记录，无法用补卡覆盖";
        }
        return false;
    }

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

bool ApprovalService::hasApprovedLeaveOverlap(int employeeId, const QDate &startDate, const QDate &endDate, int excludeRequestId, QString *errorText) const
{
    QSqlQuery query(m_db);
    query.prepare("SELECT COUNT(*) FROM leave_requests "
                  "WHERE emp_id = ? AND status = ? AND request_id != ? "
                  "AND NOT (end_date < ? OR start_date > ?)");
    query.addBindValue(employeeId);
    query.addBindValue(HR::LeaveStatus::APPROVED);
    query.addBindValue(excludeRequestId);
    query.addBindValue(startDate.toString("yyyy-MM-dd"));
    query.addBindValue(endDate.toString("yyyy-MM-dd"));

    if (!query.exec()) {
        if (errorText) {
            *errorText = query.lastError().text();
        }
        query.finish();
        return true; // fail closed
    }

    bool overlap = false;
    if (query.next()) {
        overlap = query.value(0).toInt() > 0;
    }
    query.finish();
    return overlap;
}

bool ApprovalService::hasApprovedLeaveOnDate(int employeeId, const QDate &date, QString *errorText) const
{
    QSqlQuery query(m_db);
    query.prepare("SELECT COUNT(*) FROM leave_requests "
                  "WHERE emp_id=? AND status=? AND start_date<=? AND end_date>=?");
    query.addBindValue(employeeId);
    query.addBindValue(HR::LeaveStatus::APPROVED);
    query.addBindValue(date.toString("yyyy-MM-dd"));
    query.addBindValue(date.toString("yyyy-MM-dd"));

    if (!query.exec()) {
        if (errorText) {
            *errorText = query.lastError().text();
        }
        query.finish();
        return true;
    }

    const bool exists = query.next() && query.value(0).toInt() > 0;
    query.finish();
    return exists;
}
