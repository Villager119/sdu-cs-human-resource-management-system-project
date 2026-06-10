#include "NotificationService.h"

#include "../core/Constants.h"

#include <QSqlQuery>

NotificationService::NotificationService(const QSqlDatabase &db)
    : m_db(db)
{
}

bool NotificationService::addNotification(int employeeId, const QString &title, const QString &content)
{
    QSqlQuery query(m_db);
    query.prepare("INSERT INTO notifications(emp_id,title,content) VALUES(?,?,?)");
    query.addBindValue(employeeId);
    query.addBindValue(title);
    query.addBindValue(content);

    const bool ok = query.exec();
    query.finish();
    return ok;
}

QList<int> NotificationService::activeAdminIds() const
{
    QList<int> employeeIds;
    QSqlQuery query(m_db);
    query.prepare("SELECT emp_id FROM employees WHERE role=? AND status=?");
    query.addBindValue(HR::Role::ADMIN);
    query.addBindValue(HR::EmpStatus::ACTIVE);
    query.exec();
    while (query.next()) {
        employeeIds.append(query.value(0).toInt());
    }
    query.finish();
    return employeeIds;
}

QList<int> NotificationService::activeUserIdsWithPermission(const QString &permissionKey) const
{
    QList<int> employeeIds;
    QSqlQuery query(m_db);
    query.prepare("SELECT e.emp_id FROM employees e "
                  "INNER JOIN roles r ON e.role = r.role_name "
                  "INNER JOIN role_permissions rp ON r.role_id = rp.role_id "
                  "INNER JOIN permissions p ON rp.permission_id = p.permission_id "
                  "WHERE p.permission_key=? AND e.status=?");
    query.addBindValue(permissionKey);
    query.addBindValue(HR::EmpStatus::ACTIVE);
    query.exec();
    while (query.next()) {
        employeeIds.append(query.value(0).toInt());
    }
    query.finish();

    for (int adminId : activeAdminIds()) {
        if (!employeeIds.contains(adminId)) {
            employeeIds.append(adminId);
        }
    }
    return employeeIds;
}

int NotificationService::unreadCount(int employeeId) const
{
    QSqlQuery query(m_db);
    query.prepare("SELECT COUNT(*) FROM notifications WHERE emp_id=? AND is_read=0");
    query.addBindValue(employeeId);

    int count = 0;
    if (query.exec() && query.next()) {
        count = query.value(0).toInt();
    }
    query.finish();
    return count;
}

QList<NotificationService::Notification> NotificationService::recentNotifications(int employeeId, int limit) const
{
    QList<Notification> notifications;
    QSqlQuery query(m_db);
    const int safeLimit = qMax(1, limit);
    query.prepare(QString("SELECT notif_id, title, content, created_at, is_read "
                          "FROM notifications WHERE emp_id=? ORDER BY created_at DESC LIMIT %1")
                      .arg(safeLimit));
    query.addBindValue(employeeId);

    if (query.exec()) {
        while (query.next()) {
            Notification item;
            item.id = query.value(0).toInt();
            item.title = query.value(1).toString();
            item.content = query.value(2).toString();
            item.createdAt = query.value(3).toDateTime();
            item.isRead = query.value(4).toInt() != 0;
            notifications.append(item);
        }
    }
    query.finish();
    return notifications;
}

bool NotificationService::markAllRead(int employeeId)
{
    QSqlQuery query(m_db);
    query.prepare("UPDATE notifications SET is_read=1 WHERE emp_id=?");
    query.addBindValue(employeeId);

    const bool ok = query.exec();
    query.finish();
    return ok;
}

bool NotificationService::clearRead(int employeeId)
{
    QSqlQuery query(m_db);
    query.prepare("DELETE FROM notifications WHERE emp_id=? AND is_read=1");
    query.addBindValue(employeeId);

    const bool ok = query.exec();
    query.finish();
    return ok;
}

bool NotificationService::markRead(int notificationId)
{
    QSqlQuery query(m_db);
    query.prepare("UPDATE notifications SET is_read=1 WHERE notif_id=?");
    query.addBindValue(notificationId);

    const bool ok = query.exec();
    query.finish();
    return ok;
}
