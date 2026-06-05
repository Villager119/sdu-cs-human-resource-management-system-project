#include "DbConnection.h"

#include <QByteArray>
#include <QDebug>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>

QString buildDsn(const QString &driver, const QString &server, int port,
                 const QString &database, const QString &uid, const QString &pwd)
{
    return QString("DRIVER={%1};SERVER=%2;PORT=%3;DATABASE=%4;UID=%5;PWD=%6;")
        .arg(driver, server)
        .arg(port)
        .arg(database, uid, pwd);
}

QString decodeConfigPassword(const QString &storedPassword)
{
    if (storedPassword.isEmpty()) {
        return {};
    }

    QByteArray decoded = QByteArray::fromBase64(storedPassword.toUtf8(), QByteArray::AbortOnBase64DecodingErrors);
    return decoded.isEmpty() ? storedPassword : QString::fromUtf8(decoded);
}

QSqlDatabase createClonedDatabaseConnection(const QString &prefix)
{
    static int sequence = 0;
    const QString connectionName = QString("hrms_%1_%2").arg(prefix).arg(++sequence);
    QSqlDatabase db = QSqlDatabase::cloneDatabase(QSqlDatabase::database(), connectionName);
    if (!db.open()) {
        qWarning() << "Failed to open cloned DB connection" << connectionName << db.lastError().text();
    }
    return db;
}

bool logSqlError(const QSqlQuery &query, const QString &context)
{
    const QSqlError error = query.lastError();
    if (!error.isValid()) {
        return false;
    }

    qWarning() << context << "failed:" << error.text();
    return true;
}
