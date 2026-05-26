#include "SessionManager.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QList>
#include <QDebug>

SessionManager *SessionManager::s_inst = nullptr;

SessionManager *SessionManager::instance()
{
    if (!s_inst) s_inst = new SessionManager;
    return s_inst;
}

void SessionManager::init(int empId, const QString &role, const QString &empName)
{
    auto *s = instance();
    s->empId = empId;
    s->role = role;
    s->empName = empName;

    s->reloadPermissions();
}

bool SessionManager::hasPermission(const QString &permKey) const
{
    if (role == "admin") return true;
    return permissionsSet.contains(permKey);
}

void SessionManager::reloadPermissions()
{
    permissionsSet.clear();
    if (empId == 0) return;

    {
        QSqlQuery q;
        // Sync the current user's role name from the database to handle role updates/deletions dynamically
        q.prepare("SELECT role FROM employees WHERE emp_id = ?");
        q.addBindValue(empId);
        if (q.exec() && q.next()) {
            role = q.value(0).toString();
        }
    }

    {
        QSqlQuery q;
        q.prepare("SELECT p.permission_key FROM permissions p "
                  "JOIN role_permissions rp ON p.permission_id = rp.permission_id "
                  "JOIN roles r ON rp.role_id = r.role_id "
                  "WHERE r.role_name = ?");
        q.addBindValue(role);
        if (q.exec()) {
            while (q.next()) {
                permissionsSet.insert(q.value(0).toString());
            }
        }
    }
}

