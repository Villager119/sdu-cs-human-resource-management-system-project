#ifndef AUTHSERVICE_H
#define AUTHSERVICE_H

#include <QSqlDatabase>
#include <QString>

class AuthService
{
public:
    struct LoginResult {
        bool success = false;
        bool dbError = false;
        QString errorMessage;
        int empId = 0;
        QString empName;
        QString role;
    };

    struct RecoveryIdentity {
        bool success = false;
        bool dbError = false;
        QString errorMessage;
        int empId = -1;
        QString empName;
    };

    struct OperationResult {
        enum class ErrorCode {
            None,
            InvalidOldPassword,
            DatabaseError
        };

        bool success = false;
        ErrorCode errorCode = ErrorCode::None;
        QString errorMessage;
    };

    explicit AuthService(const QSqlDatabase &db = QSqlDatabase::database());

    LoginResult authenticate(const QString &account, const QString &password) const;
    OperationResult changePassword(int empId, const QString &oldPassword, const QString &newPassword) const;
    RecoveryIdentity verifyRecoveryIdentity(int empId, const QString &name, const QString &phone) const;
    OperationResult resetRecoveredPassword(int empId, const QString &empName, const QString &newPassword) const;

    static QString hashPassword(const QString &password);

private:
    QSqlDatabase m_db;
};

#endif
