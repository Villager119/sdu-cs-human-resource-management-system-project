#ifndef SESSIONMANAGER_H
#define SESSIONMANAGER_H

#include <QString>
#include <functional>

class SessionManager
{
public:
    static SessionManager *instance();
    static void init(int empId, const QString &role, const QString &empName);

    int empId = 0;
    QString role;
    QString empName;

    using LogFn = std::function<void(const QString&, const QString&)>;
    using NotifyFn = std::function<void(int, const QString&, const QString&)>;
    LogFn logFn;
    NotifyFn notifyFn;

private:
    SessionManager() = default;
    static SessionManager *s_inst;
};

#endif
