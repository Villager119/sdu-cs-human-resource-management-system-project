#ifndef NOTIFICATIONSERVICE_H
#define NOTIFICATIONSERVICE_H

#include <QDateTime>
#include <QList>
#include <QSqlDatabase>
#include <QString>

class NotificationService
{
public:
    struct Notification {
        int id = 0;
        QString title;
        QString content;
        QDateTime createdAt;
        bool isRead = false;
    };

    explicit NotificationService(const QSqlDatabase &db = QSqlDatabase::database());

    bool addNotification(int employeeId, const QString &title, const QString &content);
    QList<int> activeAdminIds() const;
    QList<int> activeUserIdsWithPermission(const QString &permissionKey) const;
    int unreadCount(int employeeId) const;
    QList<Notification> recentNotifications(int employeeId, int limit = 10) const;
    bool markAllRead(int employeeId);
    bool clearRead(int employeeId);
    bool markRead(int notificationId);

private:
    QSqlDatabase m_db;
};

#endif
