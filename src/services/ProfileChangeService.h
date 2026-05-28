#ifndef PROFILECHANGESERVICE_H
#define PROFILECHANGESERVICE_H

#include <QSqlDatabase>
#include <QString>

class ProfileChangeService
{
public:
    struct Result {
        bool success = false;
        int employeeId = 0;
        QString fieldName;
        QString newValue;
        QString logAction;
        QString logDetails;
        QString notificationTitle;
        QString notificationContent;
        QString errorMessage;
    };

    explicit ProfileChangeService(const QSqlDatabase &db = QSqlDatabase::database());

    static QString fieldDisplayName(const QString &field);
    static bool isAllowedField(const QString &field);

    Result submitRequest(int employeeId, const QString &field, const QString &newValue, const QString &reason);
    Result approveRequest(int requestId);
    Result rejectRequest(int requestId);

private:
    struct RequestRecord {
        int employeeId = 0;
        QString field;
        QString newValue;
    };

    Result fail(const QString &message) const;
    QString currentEmployeeValue(int employeeId, const QString &field, bool *ok) const;
    RequestRecord requestRecord(int requestId, bool *ok) const;
    bool updateRequestStatus(int requestId, const QString &status, QString *errorText);

    QSqlDatabase m_db;
};

#endif
