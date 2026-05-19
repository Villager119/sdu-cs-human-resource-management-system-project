#include "SessionManager.h"

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
}
