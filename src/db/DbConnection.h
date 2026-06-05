#ifndef DBCONNECTION_H
#define DBCONNECTION_H

#include <QSqlDatabase>
#include <QString>

class QSqlQuery;

QString buildDsn(const QString &driver, const QString &server, int port,
                 const QString &database, const QString &uid, const QString &pwd);

QString decodeConfigPassword(const QString &storedPassword);

QSqlDatabase createClonedDatabaseConnection(const QString &prefix);

bool logSqlError(const QSqlQuery &query, const QString &context);

#endif
