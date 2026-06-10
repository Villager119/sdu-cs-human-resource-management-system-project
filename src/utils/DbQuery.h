#ifndef DBQUERY_H
#define DBQUERY_H

#include <QSqlError>
#include <QSqlQuery>
#include <QString>
#include <QVariant>

#include <initializer_list>

namespace DbQuery {

inline void clearError(QString *errorText)
{
    if (errorText) {
        errorText->clear();
    }
}

inline void setError(QString *errorText, const QString &message)
{
    if (errorText) {
        *errorText = message;
    }
}

inline bool execCurrent(QSqlQuery &query, QString *errorText = nullptr)
{
    clearError(errorText);
    if (!query.exec()) {
        setError(errorText, query.lastError().text());
        query.finish();
        return false;
    }
    return true;
}

inline bool exec(QSqlQuery &query, const QString &sql, QString *errorText = nullptr)
{
    clearError(errorText);
    if (!query.exec(sql)) {
        setError(errorText, query.lastError().text());
        query.finish();
        return false;
    }
    return true;
}

inline bool execPrepared(QSqlQuery &query, const QString &sql,
                         std::initializer_list<QVariant> values,
                         QString *errorText = nullptr)
{
    clearError(errorText);
    if (!query.prepare(sql)) {
        setError(errorText, query.lastError().text());
        query.finish();
        return false;
    }
    for (const QVariant &value : values) {
        query.addBindValue(value);
    }
    return execCurrent(query, errorText);
}

inline bool execAndFinish(QSqlQuery &query, const QString &sql, QString *errorText = nullptr)
{
    const bool ok = exec(query, sql, errorText);
    if (ok) {
        query.finish();
    }
    return ok;
}

inline bool execPreparedAndFinish(QSqlQuery &query, const QString &sql,
                                  std::initializer_list<QVariant> values,
                                  QString *errorText = nullptr)
{
    const bool ok = execPrepared(query, sql, values, errorText);
    if (ok) {
        query.finish();
    }
    return ok;
}

}

#endif
