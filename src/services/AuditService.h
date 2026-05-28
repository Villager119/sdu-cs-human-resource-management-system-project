#ifndef AUDITSERVICE_H
#define AUDITSERVICE_H

#include <QSqlDatabase>
#include <QString>

class AuditService
{
public:
    explicit AuditService(const QSqlDatabase &db = QSqlDatabase::database());

    qlonglong writeLog(int employeeId, const QString &employeeName,
                       const QString &action, const QString &target);
    int maxLogId() const;

private:
    QSqlDatabase m_db;
};

#endif
