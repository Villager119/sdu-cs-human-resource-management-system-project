#include "DbUtils.h"

QString buildDsn(const QString &driver, const QString &server, int port,
                 const QString &database, const QString &uid, const QString &pwd)
{
    return QString("DRIVER={%1};SERVER=%2;PORT=%3;DATABASE=%4;UID=%5;PWD=%6;")
        .arg(driver, server)
        .arg(port)
        .arg(database, uid, pwd);
}
