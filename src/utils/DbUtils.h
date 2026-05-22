#ifndef DBUTILS_H
#define DBUTILS_H

#include <QString>

QString buildDsn(const QString &driver, const QString &server, int port,
                 const QString &database, const QString &uid, const QString &pwd);

#endif
