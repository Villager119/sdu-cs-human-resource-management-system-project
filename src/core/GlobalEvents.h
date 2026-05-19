#ifndef GLOBALEVENTS_H
#define GLOBALEVENTS_H

#include <QObject>

class GlobalEvents : public QObject
{
    Q_OBJECT
public:
    static GlobalEvents *instance();
signals:
    void dataChanged();
    void dashboardRefresh();
    void auditRefresh();
};

#endif
