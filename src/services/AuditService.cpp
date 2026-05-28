#include "AuditService.h"

#include <QSqlQuery>

AuditService::AuditService(const QSqlDatabase &db)
    : m_db(db)
{
}

qlonglong AuditService::writeLog(int employeeId, const QString &employeeName,
                                 const QString &action, const QString &target)
{
    QSqlQuery query(m_db);
    query.prepare("INSERT INTO audit_logs(emp_id,emp_name,action,target,log_time) "
                  "VALUES(?,?,?,?,NOW())");
    query.addBindValue(employeeId);
    query.addBindValue(employeeName);
    query.addBindValue(action);
    query.addBindValue(target);

    qlonglong logId = -1;
    if (query.exec()) {
        logId = query.lastInsertId().toLongLong();
    }
    query.finish();
    return logId;
}

int AuditService::maxLogId() const
{
    QSqlQuery query(m_db);
    int maxId = -1;
    if (query.exec("SELECT MAX(log_id) FROM audit_logs") && query.next()) {
        maxId = query.value(0).toInt();
    }
    query.finish();
    return maxId;
}
