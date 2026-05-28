#include "ProfileChangeService.h"

#include <QMap>
#include <QSqlError>
#include <QSqlQuery>

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

ProfileChangeService::Result ProfileChangeService::submitRequest(int employeeId, const QString &field,
                                                                  const QString &newValue, const QString &reason)
{
    if (!isAllowedField(field)) {
        return fail("不允许修改该字段");
    }

    bool oldValueOk = false;
    const QString oldValue = currentEmployeeValue(employeeId, field, &oldValueOk);
    if (!oldValueOk) {
        return fail("读取员工原始信息失败");
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

ProfileChangeService::Result ProfileChangeService::approveRequest(int requestId)
{
    bool recordOk = false;
    const RequestRecord record = requestRecord(requestId, &recordOk);
    if (!recordOk) {
        return fail("未找到信息修改申请");
    }
    if (!isAllowedField(record.field)) {
        return fail("申请字段不允许修改");
    }

    if (!m_db.transaction()) {
        return fail("启动信息修改事务失败: " + m_db.lastError().text());
    }

    QSqlQuery updateEmployee(m_db);
    updateEmployee.prepare(QString("UPDATE employees SET %1=? WHERE emp_id=?").arg(record.field));
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
    if (!updateRequestStatus(requestId, "已同意", &statusError)) {
        m_db.rollback();
        return fail(statusError);
    }

    if (!m_db.commit()) {
        return fail("提交信息修改事务失败: " + m_db.lastError().text());
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
    result.notificationContent = QString("你的%1修改申请已通过审批").arg(displayName);
    return result;
}

ProfileChangeService::Result ProfileChangeService::rejectRequest(int requestId)
{
    bool recordOk = false;
    const RequestRecord record = requestRecord(requestId, &recordOk);
    if (!recordOk) {
        return fail("未找到信息修改申请");
    }

    QString statusError;
    if (!updateRequestStatus(requestId, "已拒绝", &statusError)) {
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
    result.notificationContent = QString("你的%1修改申请未通过审批").arg(displayName);
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

bool ProfileChangeService::updateRequestStatus(int requestId, const QString &status, QString *errorText)
{
    QSqlQuery query(m_db);
    query.prepare("UPDATE profile_change_requests SET status=? WHERE request_id=?");
    query.addBindValue(status);
    query.addBindValue(requestId);

    const bool ok = query.exec();
    if (!ok && errorText) {
        *errorText = query.lastError().text();
    }
    query.finish();
    return ok;
}
