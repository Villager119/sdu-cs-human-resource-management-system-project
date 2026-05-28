#include "AuthService.h"

#include "AuditService.h"

#include <QCryptographicHash>
#include <QSqlError>
#include <QSqlQuery>

AuthService::AuthService(const QSqlDatabase &db)
    : m_db(db)
{
}

QString AuthService::hashPassword(const QString &password)
{
    return QString(QCryptographicHash::hash(password.toUtf8(), QCryptographicHash::Sha256).toHex());
}

AuthService::LoginResult AuthService::authenticate(const QString &account, const QString &password) const
{
    LoginResult result;

    QSqlQuery query(m_db);
    query.prepare("SELECT emp_id,name,role FROM employees "
                  "WHERE (name=:account OR phone=:account) AND password_hash=:password");
    query.bindValue(":account", account);
    query.bindValue(":password", hashPassword(password));

    if (!query.exec()) {
        result.dbError = true;
        result.errorMessage = query.lastError().text();
    } else if (query.next()) {
        result.success = true;
        result.empId = query.value("emp_id").toInt();
        result.empName = query.value("name").toString();
        result.role = query.value("role").toString();
    }
    query.finish();
    return result;
}

AuthService::OperationResult AuthService::changePassword(int empId, const QString &oldPassword,
                                                         const QString &newPassword) const
{
    OperationResult result;

    {
        QSqlQuery query(m_db);
        query.prepare("SELECT password_hash FROM employees WHERE emp_id=?");
        query.addBindValue(empId);
        if (!query.exec()) {
            result.errorCode = OperationResult::ErrorCode::DatabaseError;
            result.errorMessage = query.lastError().text();
            query.finish();
            return result;
        }

        const bool oldPasswordOk = query.next() &&
            query.value("password_hash").toString() == hashPassword(oldPassword);
        query.finish();

        if (!oldPasswordOk) {
            result.errorCode = OperationResult::ErrorCode::InvalidOldPassword;
            result.errorMessage = "旧密码错误！";
            return result;
        }
    }

    QSqlQuery updateQuery(m_db);
    updateQuery.prepare("UPDATE employees SET password_hash=?, version = version + 1 WHERE emp_id=?");
    updateQuery.addBindValue(hashPassword(newPassword));
    updateQuery.addBindValue(empId);
    result.success = updateQuery.exec();
    if (!result.success) {
        result.errorCode = OperationResult::ErrorCode::DatabaseError;
        result.errorMessage = updateQuery.lastError().text();
    }
    updateQuery.finish();
    return result;
}

AuthService::RecoveryIdentity AuthService::verifyRecoveryIdentity(int empId, const QString &name,
                                                                  const QString &phone) const
{
    RecoveryIdentity result;

    QSqlQuery query(m_db);
    query.prepare("SELECT emp_id, name FROM employees "
                  "WHERE emp_id = :emp_id AND name = :name AND phone = :phone AND status = '在职'");
    query.bindValue(":emp_id", empId);
    query.bindValue(":name", name);
    query.bindValue(":phone", phone);

    if (!query.exec()) {
        result.dbError = true;
        result.errorMessage = query.lastError().text();
    } else if (query.next()) {
        result.success = true;
        result.empId = query.value("emp_id").toInt();
        result.empName = query.value("name").toString();
    }
    query.finish();
    return result;
}

AuthService::OperationResult AuthService::resetRecoveredPassword(int empId, const QString &empName,
                                                                 const QString &newPassword) const
{
    OperationResult result;

    QSqlQuery query(m_db);
    query.prepare("UPDATE employees SET password_hash = :pwd, version = version + 1 WHERE emp_id = :emp_id");
    query.bindValue(":pwd", hashPassword(newPassword));
    query.bindValue(":emp_id", empId);
    result.success = query.exec();
    if (!result.success) {
        result.errorCode = OperationResult::ErrorCode::DatabaseError;
        result.errorMessage = query.lastError().text();
    }
    query.finish();

    if (result.success) {
        AuditService(m_db).writeLog(empId, empName, "找回密码", "通过身份信息自主重置密码");
    }
    return result;
}
