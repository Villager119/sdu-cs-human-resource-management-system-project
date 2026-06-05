#include "AttendanceService.h"

#include "../core/Constants.h"

#include <QList>
#include <QSet>
#include <QSqlError>
#include <QSqlQuery>

static QString normalizedSqlTime(const QString &value)
{
    if (value.size() >= 8) {
        return value.left(8);
    }
    QTime parsed = QTime::fromString(value, "HH:mm");
    return parsed.isValid() ? parsed.toString("HH:mm:ss") : value;
}

static QString attendanceDateKey(int employeeId, const QDate &date)
{
    return QString("%1|%2").arg(employeeId).arg(date.toString("yyyy-MM-dd"));
}

static QString attendanceDateKey(int employeeId, const QString &dateText)
{
    return QString("%1|%2").arg(employeeId).arg(dateText.left(10));
}

AttendanceService::AttendanceService(const QSqlDatabase &db)
    : m_db(db)
{
}

AttendanceService::TodayStatus AttendanceService::todayStatus(int employeeId) const
{
    TodayStatus result;
    const ShiftTimes shift = shiftTimes(employeeId);
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
    const ShiftTimes shift = shiftTimes(employeeId);

    QSqlQuery existing(m_db);
    existing.prepare("SELECT att_id, clock_in, clock_out, status FROM attendances WHERE emp_id=? AND att_date=?");
    existing.addBindValue(employeeId);
    existing.addBindValue(dateText);
    if (existing.exec() && existing.next()) {
        const int attendanceId = existing.value(0).toInt();
        const QString currentClockIn = existing.value(1).toString();
        const QString currentClockOut = existing.value(2).toString();
        const QString currentStatus = existing.value(3).toString();
        existing.finish();

        if (currentStatus == HR::AttStatus::LEAVE) {
            return fail("当天已登记为请假，如需打卡请先调整请假审批记录");
        }
        if (!currentClockIn.isEmpty()) {
            return fail("今天已打上班卡，请勿重复");
        }
        if (!currentClockOut.isEmpty() && timeText > normalizedSqlTime(currentClockOut)) {
            return fail("上班打卡时间不能晚于已有下班打卡时间");
        }

        const QString status = statusForTimes(timeText, currentClockOut, shift, false);
        QSqlQuery update(m_db);
        update.prepare("UPDATE attendances SET clock_in=?, status=?, remark=NULL WHERE att_id=?");
        update.addBindValue(timeText);
        update.addBindValue(status);
        update.addBindValue(attendanceId);

        const bool ok = update.exec();
        const QString errorText = update.lastError().text();
        update.finish();
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
    existing.finish();

    const QString status = statusForTimes(timeText, QString(), shift, false);

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
    const ShiftTimes shift = shiftTimes(employeeId);

    QSqlQuery read(m_db);
    read.prepare("SELECT att_id, clock_in, clock_out, status FROM attendances WHERE emp_id=? AND att_date=?");
    read.addBindValue(employeeId);
    read.addBindValue(dateText);

    const bool hasRecord = read.exec() && read.next();
    const int attendanceId = hasRecord ? read.value(0).toInt() : 0;
    const QString currentClockIn = hasRecord ? read.value(1).toString() : QString();
    const QString currentClockOut = hasRecord ? read.value(2).toString() : QString();
    const QString currentStatus = hasRecord ? read.value(3).toString() : QString();
    read.finish();
    if (!hasRecord) {
        return fail("请先打上班卡");
    }
    if (currentStatus == HR::AttStatus::LEAVE) {
        return fail("当天已登记为请假，如需打卡请先调整请假审批记录");
    }
    if (currentClockIn.isEmpty()) {
        return fail("请先打上班卡");
    }
    if (!currentClockOut.isEmpty()) {
        return fail("今天已打下班卡，请勿重复");
    }
    if (timeText < normalizedSqlTime(currentClockIn)) {
        return fail("下班打卡时间不能早于上班打卡时间");
    }

    const QString status = statusForTimes(currentClockIn, timeText, shift, true);

    QSqlQuery query(m_db);
    query.prepare("UPDATE attendances SET clock_out=?, status=?, remark=NULL WHERE att_id=?");
    query.addBindValue(timeText);
    query.addBindValue(status);
    query.addBindValue(attendanceId);

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
    if (!startDate.isValid() || !endDate.isValid()) return fail("请假日期无效");
    if (startDate > endDate) return fail("结束日期不能早于开始日期");
    if (reason.isEmpty()) return fail("请假事由不能为空");
    if (reason.length() < 5) return fail("请假事由字数不能少于5个字");

    if (!m_db.transaction()) {
        return fail("启动请假提交事务失败: " + m_db.lastError().text());
    }

    QSqlQuery eq(m_db);
    eq.prepare("SELECT hire_date, status FROM employees WHERE emp_id=? FOR UPDATE");
    eq.addBindValue(employeeId);
    if (!eq.exec() || !eq.next()) {
        const QString err = eq.lastError().text();
        eq.finish();
        m_db.rollback();
        return fail(err.isEmpty() ? "未找到申请人信息" : "锁定员工记录失败: " + err);
    }
    const QString empStatus = eq.value(1).toString();
    if (empStatus != "在职") {
        eq.finish();
        m_db.rollback();
        return fail("非在职员工无法申请请假");
    }
    const QDate hireDate = eq.value(0).toDate();
    if (hireDate.isValid() && startDate < hireDate) {
        eq.finish();
        m_db.rollback();
        return fail("请假开始日期不能早于入职日期");
    }
    eq.finish();

    if (hasLeaveOverlap(employeeId, startDate, endDate)) {
        m_db.rollback();
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
    if (!ok) {
        m_db.rollback();
        return fail("提交请假申请失败: " + errorText);
    }

    if (!m_db.commit()) {
        const QString commitErr = m_db.lastError().text();
        m_db.rollback();
        return fail("提交请假申请事务失败: " + commitErr);
    }

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
    if (!date.isValid()) return fail("补卡日期无效");
    if (date > QDate::currentDate()) return fail("不能申请未来的补卡");
    if (type != "clock_in" && type != "clock_out") return fail("补卡类型无效");
    if (reason.isEmpty()) return fail("补卡事由不能为空");
    if (reason.length() < 5) return fail("补卡事由字数不能少于5个字");

    if (!m_db.transaction()) {
        return fail("启动补卡提交事务失败: " + m_db.lastError().text());
    }

    QSqlQuery eq(m_db);
    eq.prepare("SELECT hire_date, status FROM employees WHERE emp_id=? FOR UPDATE");
    eq.addBindValue(employeeId);
    if (!eq.exec() || !eq.next()) {
        const QString err = eq.lastError().text();
        eq.finish();
        m_db.rollback();
        return fail(err.isEmpty() ? "未找到申请人信息" : "锁定员工记录失败: " + err);
    }
    const QString empStatus = eq.value(1).toString();
    if (empStatus != "在职") {
        eq.finish();
        m_db.rollback();
        return fail("非在职员工无法申请补卡");
    }
    const QDate hireDate = eq.value(0).toDate();
    if (hireDate.isValid() && date < hireDate) {
        eq.finish();
        m_db.rollback();
        return fail("补卡日期不能早于入职日期");
    }
    eq.finish();

    QSqlQuery cq(m_db);
    cq.prepare("SELECT COUNT(*) FROM makeup_requests WHERE emp_id=? AND att_date=? AND request_type=? AND status IN (?, ?)");
    cq.addBindValue(employeeId);
    cq.addBindValue(date.toString("yyyy-MM-dd"));
    cq.addBindValue(type);
    cq.addBindValue(HR::LeaveStatus::PENDING);
    cq.addBindValue(HR::LeaveStatus::APPROVED);
    if (!cq.exec()) {
        const QString err = cq.lastError().text();
        cq.finish();
        m_db.rollback();
        return fail("校验重复补卡申请失败: " + err);
    }
    if (cq.next() && cq.value(0).toInt() > 0) {
        cq.finish();
        m_db.rollback();
        return fail("该日期已有同类型补卡申请，请勿重复提交");
    }
    cq.finish();

    QSqlQuery query(m_db);
    query.prepare("INSERT INTO makeup_requests(emp_id,att_date,request_type,request_time,reason,status) "
                  "VALUES(?,?,?,?,?,?)");
    query.addBindValue(employeeId);
    query.addBindValue(date.toString("yyyy-MM-dd"));
    query.addBindValue(type);
    query.addBindValue(time.toString("HH:mm:ss"));
    query.addBindValue(reason);
    query.addBindValue(HR::LeaveStatus::PENDING);

    if (!query.exec()) {
        const QString err = query.lastError().text();
        query.finish();
        m_db.rollback();
        return fail("提交补卡申请失败: " + err);
    }
    query.finish();

    if (!m_db.commit()) {
        const QString commitErr = m_db.lastError().text();
        m_db.rollback();
        return fail("提交补卡申请事务失败: " + commitErr);
    }

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

bool AttendanceService::syncApprovedLeaveToAttendance(int employeeId, const QDate &startDate,
                                                      const QDate &endDate, QString *errorText)
{
    if (!startDate.isValid() || !endDate.isValid() || startDate > endDate) {
        if (errorText) *errorText = "请假日期无效";
        return false;
    }

    for (QDate date = startDate; date <= endDate; date = date.addDays(1)) {
        QSqlQuery query(m_db);
        query.prepare("INSERT INTO attendances(emp_id, att_date, status, remark) "
                      "VALUES(?, ?, ?, '请假审批同步') "
                      "ON DUPLICATE KEY UPDATE "
                      "status=VALUES(status), "
                      "remark=CASE "
                      "WHEN clock_in IS NULL AND clock_out IS NULL THEN VALUES(remark) "
                      "WHEN remark LIKE '%原打卡已保留%' THEN remark "
                      "ELSE CONCAT_WS('; ', NULLIF(remark, ''), '请假审批同步，原打卡已保留') "
                      "END");
        query.addBindValue(employeeId);
        query.addBindValue(date.toString("yyyy-MM-dd"));
        query.addBindValue(HR::AttStatus::LEAVE);
        if (!query.exec()) {
            if (errorText) *errorText = query.lastError().text();
            query.finish();
            return false;
        }
        query.finish();
    }
    return true;
}

bool AttendanceService::backfillAttendanceRange(const QDate &startDate, const QDate &endDate,
                                                QString *errorText, int employeeId)
{
    if (!startDate.isValid() || !endDate.isValid() || startDate > endDate) {
        if (errorText) *errorText = "考勤日期范围无效";
        return false;
    }

    if (!m_db.transaction()) {
        if (errorText) *errorText = "启动考勤回填事务失败: " + m_db.lastError().text();
        return false;
    }

    struct LeaveRange {
        int employeeId = 0;
        QDate start;
        QDate end;
    };
    QList<LeaveRange> leaveRanges;
    QSet<QString> approvedLeaveDays;

    QSqlQuery leaveQuery(m_db);
    if (employeeId > 0) {
        leaveQuery.prepare("SELECT emp_id, start_date, end_date FROM leave_requests "
                           "WHERE emp_id=? AND status=? AND start_date<=? AND end_date>=?");
        leaveQuery.addBindValue(employeeId);
    } else {
        leaveQuery.prepare("SELECT emp_id, start_date, end_date FROM leave_requests "
                           "WHERE status=? AND start_date<=? AND end_date>=?");
    }
    leaveQuery.addBindValue(HR::LeaveStatus::APPROVED);
    leaveQuery.addBindValue(endDate.toString("yyyy-MM-dd"));
    leaveQuery.addBindValue(startDate.toString("yyyy-MM-dd"));
    if (!leaveQuery.exec()) {
        if (errorText) *errorText = "同步请假考勤失败: " + leaveQuery.lastError().text();
        leaveQuery.finish();
        m_db.rollback();
        return false;
    }
    while (leaveQuery.next()) {
        LeaveRange range;
        range.employeeId = leaveQuery.value(0).toInt();
        range.start = qMax(leaveQuery.value(1).toDate(), startDate);
        range.end = qMin(leaveQuery.value(2).toDate(), endDate);
        leaveRanges.append(range);

        for (QDate date = range.start; date <= range.end; date = date.addDays(1)) {
            approvedLeaveDays.insert(attendanceDateKey(range.employeeId, date));
        }
    }
    leaveQuery.finish();

    for (const auto &leaveRange : leaveRanges) {
        if (!syncApprovedLeaveToAttendance(leaveRange.employeeId, leaveRange.start, leaveRange.end, errorText)) {
            m_db.rollback();
            return false;
        }
    }

    struct EmployeeDateRange {
        int employeeId = 0;
        QDate firstDate;
        QDate lastDate;
        ShiftTimes shift;
    };
    QList<EmployeeDateRange> employeeRanges;

    QSqlQuery employeeQuery(m_db);
    if (employeeId > 0) {
        employeeQuery.prepare("SELECT e.emp_id, e.hire_date, s.start_time, s.end_time "
                              "FROM employees e "
                              "LEFT JOIN shifts s ON s.shift_id=COALESCE(e.shift_id, 1) "
                              "WHERE e.emp_id=? AND e.status=?");
        employeeQuery.addBindValue(employeeId);
        employeeQuery.addBindValue(HR::EmpStatus::ACTIVE);
    } else {
        employeeQuery.prepare("SELECT e.emp_id, e.hire_date, s.start_time, s.end_time "
                              "FROM employees e "
                              "LEFT JOIN shifts s ON s.shift_id=COALESCE(e.shift_id, 1) "
                              "WHERE e.status=?");
        employeeQuery.addBindValue(HR::EmpStatus::ACTIVE);
    }
    if (!employeeQuery.exec()) {
        if (errorText) *errorText = "读取在职员工失败: " + employeeQuery.lastError().text();
        employeeQuery.finish();
        m_db.rollback();
        return false;
    }

    while (employeeQuery.next()) {
        const int currentEmployeeId = employeeQuery.value(0).toInt();
        const QDate hireDate = employeeQuery.value(1).toDate();
        const QDate firstDate = hireDate.isValid() ? qMax(startDate, hireDate) : startDate;
        const QDate lastDate = qMin(endDate, QDate::currentDate());
        ShiftTimes shift;
        if (!employeeQuery.value(2).isNull()) {
            shift.start = normalizedSqlTime(employeeQuery.value(2).toString());
        }
        if (!employeeQuery.value(3).isNull()) {
            shift.end = normalizedSqlTime(employeeQuery.value(3).toString());
        }
        employeeRanges.append({currentEmployeeId, firstDate, lastDate, shift});
    }
    employeeQuery.finish();

    QSet<QString> existingAttendanceDays;
    QSqlQuery existingQuery(m_db);
    if (employeeId > 0) {
        existingQuery.prepare("SELECT emp_id, att_date FROM attendances "
                              "WHERE emp_id=? AND att_date BETWEEN ? AND ?");
        existingQuery.addBindValue(employeeId);
    } else {
        existingQuery.prepare("SELECT emp_id, att_date FROM attendances "
                              "WHERE att_date BETWEEN ? AND ?");
    }
    existingQuery.addBindValue(startDate.toString("yyyy-MM-dd"));
    existingQuery.addBindValue(endDate.toString("yyyy-MM-dd"));
    if (!existingQuery.exec()) {
        if (errorText) *errorText = "读取已有考勤记录失败: " + existingQuery.lastError().text();
        existingQuery.finish();
        m_db.rollback();
        return false;
    }
    while (existingQuery.next()) {
        existingAttendanceDays.insert(attendanceDateKey(existingQuery.value(0).toInt(),
                                                        existingQuery.value(1).toString()));
    }
    existingQuery.finish();

    auto isCompletedForShift = [](const QDate &date, const ShiftTimes &shift) {
        if (!date.isValid() || date > QDate::currentDate()) {
            return false;
        }
        if (date < QDate::currentDate()) {
            return true;
        }
        const QTime shiftEnd = QTime::fromString(shift.end, "HH:mm:ss");
        return shiftEnd.isValid() && QTime::currentTime() > shiftEnd;
    };

    QSqlQuery insertMissing(m_db);
    insertMissing.prepare("INSERT IGNORE INTO attendances(emp_id, att_date, status, remark) "
                          "VALUES(?, ?, ?, '系统自动生成缺卡')");
    for (const auto &employeeRange : employeeRanges) {
        for (QDate date = employeeRange.firstDate; date <= employeeRange.lastDate; date = date.addDays(1)) {
            const QString key = attendanceDateKey(employeeRange.employeeId, date);
            if (!isWorkday(date) || !isCompletedForShift(date, employeeRange.shift)
                || approvedLeaveDays.contains(key) || existingAttendanceDays.contains(key)) {
                continue;
            }

            insertMissing.bindValue(0, employeeRange.employeeId);
            insertMissing.bindValue(1, date.toString("yyyy-MM-dd"));
            insertMissing.bindValue(2, HR::AttStatus::MISSING);
            if (!insertMissing.exec()) {
                if (errorText) *errorText = "生成缺卡记录失败: " + insertMissing.lastError().text();
                insertMissing.finish();
                m_db.rollback();
                return false;
            }
            existingAttendanceDays.insert(key);
        }
    }
    insertMissing.finish();

    struct AttendanceRecord {
        int attendanceId = 0;
        int employeeId = 0;
        QDate date;
        QString clockIn;
        QString clockOut;
        QString status;
        ShiftTimes shift;
    };
    QList<AttendanceRecord> recordsToRefresh;

    const QDate refreshEndDate = qMin(endDate, QDate::currentDate());
    if (startDate <= refreshEndDate) {
        QSqlQuery attendanceQuery(m_db);
        if (employeeId > 0) {
            attendanceQuery.prepare("SELECT a.att_id, a.emp_id, a.att_date, a.clock_in, a.clock_out, a.status, "
                                    "s.start_time, s.end_time "
                                    "FROM attendances a "
                                    "LEFT JOIN employees e ON e.emp_id=a.emp_id "
                                    "LEFT JOIN shifts s ON s.shift_id=COALESCE(e.shift_id, 1) "
                                    "WHERE a.emp_id=? AND a.att_date BETWEEN ? AND ? AND a.status<>?");
            attendanceQuery.addBindValue(employeeId);
        } else {
            attendanceQuery.prepare("SELECT a.att_id, a.emp_id, a.att_date, a.clock_in, a.clock_out, a.status, "
                                    "s.start_time, s.end_time "
                                    "FROM attendances a "
                                    "LEFT JOIN employees e ON e.emp_id=a.emp_id "
                                    "LEFT JOIN shifts s ON s.shift_id=COALESCE(e.shift_id, 1) "
                                    "WHERE a.att_date BETWEEN ? AND ? AND a.status<>?");
        }
        attendanceQuery.addBindValue(startDate.toString("yyyy-MM-dd"));
        attendanceQuery.addBindValue(refreshEndDate.toString("yyyy-MM-dd"));
        attendanceQuery.addBindValue(HR::AttStatus::LEAVE);
        if (!attendanceQuery.exec()) {
            if (errorText) *errorText = "读取待刷新考勤记录失败: " + attendanceQuery.lastError().text();
            attendanceQuery.finish();
            m_db.rollback();
            return false;
        }
        while (attendanceQuery.next()) {
            AttendanceRecord record;
            record.attendanceId = attendanceQuery.value(0).toInt();
            record.employeeId = attendanceQuery.value(1).toInt();
            record.date = attendanceQuery.value(2).toDate();
            record.clockIn = attendanceQuery.value(3).toString();
            record.clockOut = attendanceQuery.value(4).toString();
            record.status = attendanceQuery.value(5).toString();
            if (!attendanceQuery.value(6).isNull()) {
                record.shift.start = normalizedSqlTime(attendanceQuery.value(6).toString());
            }
            if (!attendanceQuery.value(7).isNull()) {
                record.shift.end = normalizedSqlTime(attendanceQuery.value(7).toString());
            }
            recordsToRefresh.append(record);
        }
        attendanceQuery.finish();
    }

    QSqlQuery updateStatus(m_db);
    updateStatus.prepare("UPDATE attendances SET status=? WHERE att_id=?");
    for (const AttendanceRecord &record : recordsToRefresh) {
        const QString status = statusForTimes(record.clockIn, record.clockOut, record.shift,
                                              isCompletedForShift(record.date, record.shift));
        if (status == record.status) {
            continue;
        }

        updateStatus.bindValue(0, status);
        updateStatus.bindValue(1, record.attendanceId);
        if (!updateStatus.exec()) {
            if (errorText) *errorText = "刷新考勤状态失败: " + updateStatus.lastError().text();
            updateStatus.finish();
            m_db.rollback();
            return false;
        }
    }
    updateStatus.finish();

    if (!m_db.commit()) {
        if (errorText) *errorText = "提交考勤回填事务失败: " + m_db.lastError().text();
        m_db.rollback();
        return false;
    }
    return true;
}

bool AttendanceService::refreshAttendanceStatus(int attendanceId, QString *errorText)
{
    QSqlQuery readQuery(m_db);
    readQuery.prepare("SELECT emp_id, att_date, clock_in, clock_out, status FROM attendances WHERE att_id=?");
    readQuery.addBindValue(attendanceId);
    if (!readQuery.exec() || !readQuery.next()) {
        if (errorText) {
            *errorText = readQuery.lastError().isValid() ? readQuery.lastError().text() : "考勤记录不存在";
        }
        readQuery.finish();
        return false;
    }

    const int employeeId = readQuery.value(0).toInt();
    const QDate date = readQuery.value(1).toDate();
    const QString clockIn = readQuery.value(2).toString();
    const QString clockOut = readQuery.value(3).toString();
    const QString currentStatus = readQuery.value(4).toString();
    readQuery.finish();

    if (currentStatus == HR::AttStatus::LEAVE) {
        return true;
    }

    const QString status = statusForTimes(clockIn, clockOut, shiftTimes(employeeId),
                                          isCompletedAttendanceDate(employeeId, date));

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

AttendanceService::ShiftTimes AttendanceService::shiftTimes(int employeeId) const
{
    ShiftTimes result;
    QSqlQuery query(m_db);
    if (employeeId > 0) {
        query.prepare("SELECT s.start_time, s.end_time FROM employees e "
                      "JOIN shifts s ON s.shift_id=COALESCE(e.shift_id, 1) "
                      "WHERE e.emp_id=?");
        query.addBindValue(employeeId);
    } else {
        query.prepare("SELECT start_time, end_time FROM shifts WHERE shift_id=1");
    }
    if (query.exec() && query.next()) {
        result.start = normalizedSqlTime(query.value(0).toString());
        result.end = normalizedSqlTime(query.value(1).toString());
    }
    query.finish();
    return result;
}

QString AttendanceService::statusForTimes(const QString &clockIn, const QString &clockOut,
                                          const ShiftTimes &shift, bool requireCompletePunches) const
{
    if (requireCompletePunches && (clockIn.isEmpty() || clockOut.isEmpty())) {
        return HR::AttStatus::MISSING;
    }

    const bool late = !clockIn.isEmpty() && normalizedSqlTime(clockIn) > shift.start;
    const bool early = !clockOut.isEmpty() && normalizedSqlTime(clockOut) < shift.end;
    if (late && early) return HR::AttStatus::LATE_EARLY;
    if (late) return HR::AttStatus::LATE;
    if (early) return HR::AttStatus::EARLY;
    return HR::AttStatus::NORMAL;
}

bool AttendanceService::isCompletedAttendanceDate(int employeeId, const QDate &date) const
{
    if (!date.isValid() || date > QDate::currentDate()) {
        return false;
    }
    if (date < QDate::currentDate()) {
        return true;
    }
    const QTime shiftEnd = QTime::fromString(shiftTimes(employeeId).end, "HH:mm:ss");
    return shiftEnd.isValid() && QTime::currentTime() > shiftEnd;
}

bool AttendanceService::isWorkday(const QDate &date) const
{
    return date.isValid() && date.dayOfWeek() >= 1 && date.dayOfWeek() <= 5;
}

bool AttendanceService::hasApprovedLeaveOnDate(int employeeId, const QDate &date) const
{
    QSqlQuery query(m_db);
    query.prepare("SELECT COUNT(*) FROM leave_requests "
                  "WHERE emp_id=? AND status=? AND start_date<=? AND end_date>=?");
    query.addBindValue(employeeId);
    query.addBindValue(HR::LeaveStatus::APPROVED);
    query.addBindValue(date.toString("yyyy-MM-dd"));
    query.addBindValue(date.toString("yyyy-MM-dd"));

    const bool exists = query.exec() && query.next() && query.value(0).toInt() > 0;
    query.finish();
    return exists;
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
