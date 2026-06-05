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
    static bool validateFieldValue(const QString &field, const QString &value, QString *errorMessage = nullptr);

    Result submitRequest(int employeeId, const QString &field, const QString &newValue, const QString &reason);
    Result approveRequest(int requestId, const QString &comment = QString(), int reviewerId = 0);
    Result rejectRequest(int requestId, const QString &comment = QString(), int reviewerId = 0);

private:
    struct RequestRecord {
        int employeeId = 0;
        QString field;
        QString newValue;
    };

    Result fail(const QString &message) const;
    QString currentEmployeeValue(int employeeId, const QString &field, bool *ok) const;
    bool hasPendingRequest(int employeeId, const QString &field, QString *errorText) const;
    bool ensurePhoneAvailable(int employeeId, const QString &phone, QString *errorText) const;
    bool ensurePositionAvailableForEmployee(int employeeId, const QString &position, QString *errorText) const;
    RequestRecord requestRecord(int requestId, bool *ok) const;
    bool updateRequestStatus(int requestId, const QString &status, const QString &comment,
                             int reviewerId, QString *errorText);

    QSqlDatabase m_db;
};

#endif
