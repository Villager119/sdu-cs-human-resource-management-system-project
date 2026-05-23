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

    // Create tables if they don't exist
    QSqlQuery q;

    // Migrate employees.role column if it is an enum to support dynamic role names
    q.exec("SHOW COLUMNS FROM employees LIKE 'role'");
    if (q.next()) {
        QString type = q.value(1).toString().toLower();
        if (type.contains("enum")) {
            if (!q.exec("ALTER TABLE employees MODIFY COLUMN role VARCHAR(50) DEFAULT 'user'")) {
                qDebug() << "Failed to alter employees.role to VARCHAR:" << q.lastError().text();
            }
        }
    }

    if (!q.exec("CREATE TABLE IF NOT EXISTS roles ("
                "role_id INT AUTO_INCREMENT PRIMARY KEY,"
                "role_name VARCHAR(50) NOT NULL UNIQUE"
                ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4")) {
        qDebug() << "Failed to create roles table:" << q.lastError().text();
    }

    if (!q.exec("CREATE TABLE IF NOT EXISTS permissions ("
                "permission_id INT AUTO_INCREMENT PRIMARY KEY,"
                "permission_key VARCHAR(50) NOT NULL UNIQUE,"
                "permission_name VARCHAR(100) NOT NULL"
                ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4")) {
        qDebug() << "Failed to create permissions table:" << q.lastError().text();
    }

    if (!q.exec("CREATE TABLE IF NOT EXISTS role_permissions ("
                "role_id INT NOT NULL,"
                "permission_id INT NOT NULL,"
                "PRIMARY KEY (role_id, permission_id),"
                "FOREIGN KEY (role_id) REFERENCES roles(role_id) ON DELETE CASCADE,"
                "FOREIGN KEY (permission_id) REFERENCES permissions(permission_id) ON DELETE CASCADE"
                ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4")) {
        qDebug() << "Failed to create role_permissions table:" << q.lastError().text();
    }

    // Populate default roles
    q.exec("INSERT IGNORE INTO roles (role_name) VALUES ('admin'), ('user')");

    // Populate default permissions
    struct Perm { QString key; QString name; };
    QList<Perm> defaultPerms = {
        {"view_dashboard", "查看仪表盘"},
        {"view_reports", "查看统计图表"},
        {"manage_employees", "员工管理"},
        {"request_profile_change", "申请个人信息修改"},
        {"approve_profile_change", "审批信息修改申请"},
        {"apply_leave_makeup", "申请打卡/补卡"},
        {"approve_makeup", "审批打卡/补卡"},
        {"apply_leave", "申请请假"},
        {"approve_leave", "审批请假"},
        {"view_personal_payroll", "查看个人薪资"},
        {"calculate_payroll", "薪资计算与发放"},
        {"view_personal_performance", "查看个人绩效"},
        {"evaluate_performance", "员工绩效评估"},
        {"manage_tax_config", "社保与个税配置"},
        {"manage_org", "组织架构管理"},
        {"view_audit_logs", "查看审计日志"},
        {"manage_rbac", "系统权限管理"}
    };

    for (const auto &p : defaultPerms) {
        QSqlQuery pq;
        pq.prepare("INSERT IGNORE INTO permissions (permission_key, permission_name) VALUES (?, ?)");
        pq.addBindValue(p.key);
        pq.addBindValue(p.name);
        if (!pq.exec()) {
            // If INSERT IGNORE fails, maybe it already exists, which is fine
        }
    }

    // Assign all permissions to admin, and basic permissions to user
    int adminRoleId = -1;
    int userRoleId = -1;
    q.exec("SELECT role_id, role_name FROM roles");
    while (q.next()) {
        int rid = q.value(0).toInt();
        QString rname = q.value(1).toString();
        if (rname == "admin") adminRoleId = rid;
        else if (rname == "user") userRoleId = rid;
    }

    if (adminRoleId != -1) {
        q.exec(QString("INSERT IGNORE INTO role_permissions (role_id, permission_id) "
                       "SELECT %1, permission_id FROM permissions").arg(adminRoleId));
    }

    if (userRoleId != -1) {
        QList<QString> userPerms = {
            "view_dashboard",
            "request_profile_change",
            "apply_leave_makeup",
            "apply_leave",
            "view_personal_payroll",
            "view_personal_performance"
        };
        for (const auto &pk : userPerms) {
            QSqlQuery uq;
            uq.prepare("INSERT IGNORE INTO role_permissions (role_id, permission_id) "
                       "SELECT ?, permission_id FROM permissions WHERE permission_key = ?");
            uq.bindValue(0, userRoleId);
            uq.bindValue(1, pk);
            uq.exec();
        }
    }

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

    QSqlQuery q;
    // Sync the current user's role name from the database to handle role updates/deletions dynamically
    q.prepare("SELECT role FROM employees WHERE emp_id = ?");
    q.addBindValue(empId);
    if (q.exec() && q.next()) {
        role = q.value(0).toString();
    }

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

