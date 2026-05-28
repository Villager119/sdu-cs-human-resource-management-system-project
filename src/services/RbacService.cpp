#include "RbacService.h"

#include <QRegularExpression>
#include <QSqlError>
#include <QSqlQuery>

RbacService::RbacService(const QSqlDatabase &db)
    : m_db(db)
{
}

QVector<QString> RbacService::roles() const
{
    QVector<QString> roleNames;
    QSqlQuery query(m_db);
    if (query.exec("SELECT role_name FROM roles ORDER BY role_id")) {
        while (query.next()) {
            roleNames.append(query.value(0).toString());
        }
    }
    query.finish();
    return roleNames;
}

QVector<RbacService::Permission> RbacService::permissions() const
{
    QVector<Permission> items;
    QSqlQuery query(m_db);
    if (query.exec("SELECT permission_key, permission_name FROM permissions ORDER BY permission_id")) {
        while (query.next()) {
            items.append({query.value(0).toString(), query.value(1).toString()});
        }
    }
    query.finish();
    return items;
}

QSet<QString> RbacService::permissionKeysForRole(const QString &roleName) const
{
    QSet<QString> keys;
    QSqlQuery query(m_db);
    query.prepare("SELECT p.permission_key FROM permissions p "
                  "JOIN role_permissions rp ON p.permission_id = rp.permission_id "
                  "JOIN roles r ON rp.role_id = r.role_id "
                  "WHERE r.role_name = ?");
    query.addBindValue(roleName);
    if (query.exec()) {
        while (query.next()) {
            keys.insert(query.value(0).toString());
        }
    }
    query.finish();
    return keys;
}

int RbacService::roleId(const QString &roleName) const
{
    QSqlQuery query(m_db);
    query.prepare("SELECT role_id FROM roles WHERE role_name = ?");
    query.addBindValue(roleName);

    int id = -1;
    if (query.exec() && query.next()) {
        id = query.value(0).toInt();
    }
    query.finish();
    return id;
}

RbacService::Result RbacService::addRole(const QString &roleName)
{
    if (roleName.isEmpty()) {
        return fail("角色名称不能为空！");
    }

    QRegularExpression re("^[a-zA-Z0-9_]+$");
    if (!re.match(roleName).hasMatch()) {
        return fail("角色名称只能包含字母、数字和下划线！");
    }

    QSqlQuery query(m_db);
    query.prepare("INSERT INTO roles (role_name) VALUES (?)");
    query.addBindValue(roleName);

    const bool ok = query.exec();
    const QString errorText = query.lastError().text();
    query.finish();
    if (!ok) {
        return fail("无法添加角色，可能已存在同名角色！\n" + errorText);
    }

    Result result;
    result.success = true;
    return result;
}

RbacService::Result RbacService::deleteRole(const QString &roleName)
{
    if (roleName == "admin" || roleName == "user") {
        return fail("系统默认角色无法删除！");
    }

    if (!m_db.transaction()) {
        return fail("启动角色删除事务失败: " + m_db.lastError().text());
    }

    QSqlQuery updateEmployees(m_db);
    updateEmployees.prepare("UPDATE employees SET role = 'user', version = version + 1 WHERE role = ?");
    updateEmployees.addBindValue(roleName);
    if (!updateEmployees.exec()) {
        const QString errorText = updateEmployees.lastError().text();
        updateEmployees.finish();
        m_db.rollback();
        return fail(errorText);
    }
    updateEmployees.finish();

    QSqlQuery deleteRoleQuery(m_db);
    deleteRoleQuery.prepare("DELETE FROM roles WHERE role_name = ?");
    deleteRoleQuery.addBindValue(roleName);
    if (!deleteRoleQuery.exec()) {
        const QString errorText = deleteRoleQuery.lastError().text();
        deleteRoleQuery.finish();
        m_db.rollback();
        return fail(errorText);
    }
    deleteRoleQuery.finish();

    if (!m_db.commit()) {
        const QString commitErr = m_db.lastError().text();
        m_db.rollback();
        return fail("提交角色删除事务失败: " + commitErr);
    }

    Result result;
    result.success = true;
    return result;
}

RbacService::Result RbacService::saveRolePermissions(int roleId, const QSet<QString> &permissionKeys)
{
    if (roleId <= 0) {
        return fail("未找到对应的角色ID！");
    }

    if (!m_db.transaction()) {
        return fail("启动权限保存事务失败: " + m_db.lastError().text());
    }

    QSqlQuery deleteMappings(m_db);
    deleteMappings.prepare("DELETE FROM role_permissions WHERE role_id = ?");
    deleteMappings.addBindValue(roleId);
    if (!deleteMappings.exec()) {
        const QString errorText = deleteMappings.lastError().text();
        deleteMappings.finish();
        m_db.rollback();
        return fail("清除旧权限映射失败！\n" + errorText);
    }
    deleteMappings.finish();

    for (const QString &permissionKey : permissionKeys) {
        QSqlQuery insertMapping(m_db);
        insertMapping.prepare("INSERT INTO role_permissions (role_id, permission_id) "
                              "SELECT ?, permission_id FROM permissions WHERE permission_key = ?");
        insertMapping.addBindValue(roleId);
        insertMapping.addBindValue(permissionKey);
        if (!insertMapping.exec()) {
            const QString errorText = insertMapping.lastError().text();
            insertMapping.finish();
            m_db.rollback();
            return fail("保存权限映射失败！\n" + errorText);
        }
        insertMapping.finish();
    }

    if (!m_db.commit()) {
        const QString commitErr = m_db.lastError().text();
        m_db.rollback();
        return fail("提交权限保存事务失败: " + commitErr);
    }

    Result result;
    result.success = true;
    return result;
}

RbacService::Result RbacService::fail(const QString &message) const
{
    Result result;
    result.errorMessage = message;
    return result;
}
