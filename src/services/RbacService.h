#ifndef RBACSERVICE_H
#define RBACSERVICE_H

#include <QSet>
#include <QSqlDatabase>
#include <QString>
#include <QVector>

class RbacService
{
public:
    struct Permission {
        QString key;
        QString name;
    };

    struct Result {
        bool success = false;
        QString errorMessage;
    };

    explicit RbacService(const QSqlDatabase &db = QSqlDatabase::database());

    QVector<QString> roles() const;
    QVector<Permission> permissions() const;
    QSet<QString> permissionKeysForRole(const QString &roleName) const;
    int roleId(const QString &roleName) const;
    Result addRole(const QString &roleName);
    Result deleteRole(const QString &roleName);
    Result saveRolePermissions(int roleId, const QSet<QString> &permissionKeys);

private:
    Result fail(const QString &message) const;

    QSqlDatabase m_db;
};

#endif
