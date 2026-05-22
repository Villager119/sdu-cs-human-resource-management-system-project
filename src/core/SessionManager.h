#ifndef SESSIONMANAGER_H
#define SESSIONMANAGER_H

#include <QString>
#include <QSet>
#include <functional>

class SessionManager
{
public:
    static SessionManager *instance();
    static void init(int empId, const QString &role, const QString &empName);

    int empId = 0;
    QString role;
    QString empName;

    QSet<QString> permissionsSet;

    bool hasPermission(const QString &permKey) const;
    void reloadPermissions();

private:
    SessionManager() = default;
    static SessionManager *s_inst;
};

#endif

