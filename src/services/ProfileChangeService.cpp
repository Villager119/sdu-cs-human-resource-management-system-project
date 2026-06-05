#include "ProfileChangeService.h"

#include "EmployeeService.h"

#include <QMap>
#include <QMetaType>
#include <QRegularExpression>
#include <QSqlError>
#include <QSqlQuery>
#include <QStringList>
#include <QVariant>

ProfileChangeService::ProfileChangeService(const QSqlDatabase &db)
    : m_db(db)
{
}

QString ProfileChangeService::fieldDisplayName(const QString &field)
{
    static QMap<QString, QString> map = {
        {"phone", "联系电话"},
        {"education", "学历"},
        {"marital_status", "婚姻状况"},
        {"position", "岗位"},
        {"gender", "性别"}
    };
    return map.value(field, field);
}

bool ProfileChangeService::isAllowedField(const QString &field)
{
    static const QStringList fields = {"phone", "education", "marital_status", "position", "gender"};
    return fields.contains(field);
}

bool ProfileChangeService::validateFieldValue(const QString &field, const QString &value, QString *errorMessage)
{
    auto fail = [errorMessage](const QString &message) {
        if (errorMessage) {
            *errorMessage = message;
        }
        return false;
    };

    const QString trimmed = value.trimmed();
    if (trimmed.isEmpty()) {
        return fail("新值不能为空");
    }

    if (field == "phone") {
        if (!QRegularExpression("^\\d{11}$").match(trimmed).hasMatch()) {
            return fail("联系电话格式不正确（必须为 11 位数字）");
        }
        return true;
    }
    if (field == "education") {
        if (!QStringList({"大专", "本科", "硕士", "博士"}).contains(trimmed)) {
            return fail("学历必须是 大专/本科/硕士/博士 之一");
        }
        return true;
    }
    if (field == "marital_status") {
        if (!QStringList({"未婚", "已婚", "离异"}).contains(trimmed)) {
            return fail("婚姻状况必须是 未婚/已婚/离异 之一");
        }
        return true;
    }
    if (field == "gender") {
        if (trimmed != "男" && trimmed != "女") {
            return fail("性别必须是 男 或 女");
        }
        return true;
    }
    if (field == "position") {
        if (trimmed.length() > 50) {
            return fail("岗位长度不能超过 50 个字符");
        }
        return true;
    }

    return fail("不允许修改该字段");
}

ProfileChangeService::Result ProfileChangeService::submitRequest(int employeeId, const QString &field,
                                                                  const QString &newValue, const QString &reason)
{
    if (!isAllowedField(field)) {
        return fail("不允许修改该字段");
    }
    QString validationError;
    if (!validateFieldValue(field, newValue, &validationError)) {
        return fail(validationError);
    }
    QString pendingError;
    if (hasPendingRequest(employeeId, field, &pendingError)) {
        return fail(pendingError.isEmpty() ? "该字段已有待审批的信息修改申请，请勿重复提交" : pendingError);
    }

    bool oldValueOk = false;
    const QString oldValue = currentEmployeeValue(employeeId, field, &oldValueOk);
    if (!oldValueOk) {
        return fail("读取员工原始信息失败");
    }
    if (field == "phone") {
        QString phoneError;
        if (!ensurePhoneAvailable(employeeId, newValue, &phoneError)) {
            return fail(phoneError);
        }
    }
    if (field == "position") {
        QString positionError;
        if (!ensurePositionAvailableForEmployee(employeeId, newValue, &positionError)) {
            return fail(positionError);
        }
    }

    QSqlQuery query(m_db);
    query.prepare("INSERT INTO profile_change_requests(emp_id,field_name,old_value,new_value,reason) "
                  "VALUES(?,?,?,?,?)");
    query.addBindValue(employeeId);
    query.addBindValue(field);
    query.addBindValue(oldValue);
    query.addBindValue(newValue);
    query.addBindValue(reason);

    const bool ok = query.exec();
    const QString errorText = query.lastError().text();
    query.finish();
    if (!ok) {
        return fail(errorText);
    }

    const QString displayName = fieldDisplayName(field);
    Result result;
    result.success = true;
    result.employeeId = employeeId;
    result.fieldName = displayName;
    result.newValue = newValue;
    result.logAction = "提交信息修改申请";
    result.logDetails = displayName + " → " + newValue;
    result.notificationTitle = "信息修改申请";
    result.notificationContent = QString("员工申请修改%1为%2").arg(displayName, newValue);
    return result;
}

ProfileChangeService::Result ProfileChangeService::approveRequest(int requestId,
                                                                   const QString &comment,
                                                                   int reviewerId)
{
    bool recordOk = false;
    const RequestRecord record = requestRecord(requestId, &recordOk);
    if (!recordOk) {
        return fail("未找到信息修改申请");
    }
    if (!isAllowedField(record.field)) {
        return fail("申请字段不允许修改");
    }
    QString validationError;
    if (!validateFieldValue(record.field, record.newValue, &validationError)) {
        return fail(validationError);
    }

    if (!m_db.transaction()) {
        return fail("启动信息修改事务失败: " + m_db.lastError().text());
    }

    if (record.field == "phone") {
        QString phoneError;
        if (!ensurePhoneAvailable(record.employeeId, record.newValue, &phoneError)) {
            m_db.rollback();
            return fail(phoneError);
        }
    }
    if (record.field == "position") {
        QString positionError;
        if (!ensurePositionAvailableForEmployee(record.employeeId, record.newValue, &positionError)) {
            m_db.rollback();
            return fail(positionError);
        }
    }

    QSqlQuery updateEmployee(m_db);
    updateEmployee.prepare(QString("UPDATE employees SET %1=?, version = version + 1 WHERE emp_id=?").arg(record.field));
    updateEmployee.addBindValue(record.newValue);
    updateEmployee.addBindValue(record.employeeId);
    if (!updateEmployee.exec()) {
        const QString errorText = updateEmployee.lastError().text();
        updateEmployee.finish();
        m_db.rollback();
        return fail(errorText);
    }
    updateEmployee.finish();

    QString statusError;
    if (!updateRequestStatus(requestId, "已同意", comment.trimmed(), reviewerId, &statusError)) {
        m_db.rollback();
        return fail(statusError);
    }

    if (!m_db.commit()) {
        const QString commitErr = m_db.lastError().text();
        m_db.rollback();
        return fail("提交信息修改事务失败: " + commitErr);
    }

    const QString displayName = fieldDisplayName(record.field);
    Result result;
    result.success = true;
    result.employeeId = record.employeeId;
    result.fieldName = displayName;
    result.newValue = record.newValue;
    result.logAction = "同意信息修改";
    result.logDetails = QString("申请#%1 %2→%3").arg(requestId).arg(displayName, record.newValue);
    result.notificationTitle = "信息修改已批准";
    result.notificationContent = comment.trimmed().isEmpty()
        ? QString("你的%1修改申请已通过审批").arg(displayName)
        : QString("你的%1修改申请已通过审批，审批意见：%2").arg(displayName, comment.trimmed());
    return result;
}

ProfileChangeService::Result ProfileChangeService::rejectRequest(int requestId,
                                                                  const QString &comment,
                                                                  int reviewerId)
{
    if (comment.trimmed().isEmpty()) {
        return fail("拒绝申请时必须填写审批意见，方便员工了解处理原因");
    }

    bool recordOk = false;
    const RequestRecord record = requestRecord(requestId, &recordOk);
    if (!recordOk) {
        return fail("未找到信息修改申请");
    }

    QString statusError;
    if (!updateRequestStatus(requestId, "已拒绝", comment.trimmed(), reviewerId, &statusError)) {
        return fail(statusError);
    }

    const QString displayName = fieldDisplayName(record.field);
    Result result;
    result.success = true;
    result.employeeId = record.employeeId;
    result.fieldName = displayName;
    result.logAction = "拒绝信息修改";
    result.logDetails = QString("申请#%1 %2").arg(requestId).arg(displayName);
    result.notificationTitle = "信息修改已拒绝";
    result.notificationContent = QString("你的%1修改申请未通过审批，原因：%2").arg(displayName, comment.trimmed());
    return result;
}

ProfileChangeService::Result ProfileChangeService::fail(const QString &message) const
{
    Result result;
    result.errorMessage = message;
    return result;
}

QString ProfileChangeService::currentEmployeeValue(int employeeId, const QString &field, bool *ok) const
{
    if (ok) *ok = false;

    QSqlQuery query(m_db);
    query.prepare(QString("SELECT %1 FROM employees WHERE emp_id=?").arg(field));
    query.addBindValue(employeeId);

    QString value;
    if (query.exec() && query.next()) {
        value = query.value(0).toString();
        if (ok) *ok = true;
    }
    query.finish();
    return value;
}

bool ProfileChangeService::hasPendingRequest(int employeeId, const QString &field, QString *errorText) const
{
    QSqlQuery query(m_db);
    query.prepare("SELECT COUNT(*) FROM profile_change_requests "
                  "WHERE emp_id=? AND field_name=? AND status='待审批'");
    query.addBindValue(employeeId);
    query.addBindValue(field);

    if (!query.exec()) {
        if (errorText) {
            *errorText = "校验重复信息修改申请失败: " + query.lastError().text();
        }
        query.finish();
        return true;
    }

    const bool exists = query.next() && query.value(0).toInt() > 0;
    query.finish();
    return exists;
}

bool ProfileChangeService::ensurePhoneAvailable(int employeeId, const QString &phone, QString *errorText) const
{
    return EmployeeService(m_db).phoneAvailableForEmployee(phone, employeeId, errorText);
}

bool ProfileChangeService::ensurePositionAvailableForEmployee(int employeeId, const QString &position, QString *errorText) const
{
    QSqlQuery query(m_db);
    query.prepare("SELECT department FROM employees WHERE emp_id=?");
    query.addBindValue(employeeId);
    if (!query.exec() || !query.next()) {
        if (errorText) {
            *errorText = query.lastError().isValid() ? query.lastError().text() : "员工不存在";
        }
        query.finish();
        return false;
    }
    const QString department = query.value(0).toString().trimmed();
    query.finish();

    if (department.isEmpty()) {
        if (errorText) {
            *errorText = "员工未设置部门，无法校验岗位";
        }
        return false;
    }

    QSqlQuery check(m_db);
    check.prepare("SELECT COUNT(*) FROM job_salary_standards s "
                  "JOIN departments d ON d.dept_id=s.dept_id "
                  "WHERE d.dept_name=? AND s.position=? AND s.enabled=1");
    check.addBindValue(department);
    check.addBindValue(position.trimmed());
    const bool ok = check.exec() && check.next();
    const bool exists = ok && check.value(0).toInt() > 0;
    if (!ok && errorText) {
        *errorText = "校验岗位失败: " + check.lastError().text();
    }
    if (ok && !exists && errorText) {
        *errorText = QString("当前部门 '%1' 下未配置岗位 '%2'，请先在组织管理中配置岗位薪资标准")
            .arg(department, position.trimmed());
    }
    check.finish();
    return ok && exists;
}

ProfileChangeService::RequestRecord ProfileChangeService::requestRecord(int requestId, bool *ok) const
{
    if (ok) *ok = false;

    QSqlQuery query(m_db);
    query.prepare("SELECT emp_id, field_name, new_value FROM profile_change_requests WHERE request_id=?");
    query.addBindValue(requestId);

    RequestRecord record;
    if (query.exec() && query.next()) {
        record.employeeId = query.value(0).toInt();
        record.field = query.value(1).toString();
        record.newValue = query.value(2).toString();
        if (ok) *ok = true;
    }
    query.finish();
    return record;
}

bool ProfileChangeService::updateRequestStatus(int requestId, const QString &status,
                                               const QString &comment, int reviewerId,
                                               QString *errorText)
{
    QSqlQuery query(m_db);
    query.prepare("UPDATE profile_change_requests SET status=?, reviewer_id=?, reviewed_at=NOW(), review_comment=? "
                  "WHERE request_id=? AND status='待审批'");
    query.addBindValue(status);
    query.addBindValue(reviewerId > 0 ? QVariant(reviewerId) : QVariant(QMetaType(QMetaType::Int)));
    query.addBindValue(comment);
    query.addBindValue(requestId);

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
