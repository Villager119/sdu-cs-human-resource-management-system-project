#include "DbMigration.h"

#include <QDebug>
#include <QSqlError>
#include <QSqlQuery>

bool ensureDbColumn(const QString &table, const QString &column, const QString &definition)
{
    QSqlQuery ck;
    if (ck.exec(QString("SHOW COLUMNS FROM %1 LIKE '%2'").arg(table, column)) && ck.next()) {
        ck.finish();
        return true;
    }
    ck.finish();

    QSqlQuery alter;
    if (!alter.exec(QString("ALTER TABLE %1 ADD COLUMN %2 %3").arg(table, column, definition))) {
        qDebug() << "Failed to add column" << column << "to" << table << ":" << alter.lastError().text();
        alter.finish();
        return false;
    }
    alter.finish();
    return true;
}

bool ensureDbIndex(const QString &table, const QString &indexName, const QString &columns)
{
    QSqlQuery ck;
    if (ck.exec(QString("SHOW INDEX FROM %1 WHERE Key_name = '%2'").arg(table, indexName)) && ck.next()) {
        ck.finish();
        return true;
    }
    ck.finish();

    QSqlQuery alter;
    if (!alter.exec(QString("ALTER TABLE %1 ADD INDEX %2 (%3)").arg(table, indexName, columns))) {
        qDebug() << "Failed to add index" << indexName << "to" << table << ":" << alter.lastError().text();
        alter.finish();
        return false;
    }
    alter.finish();
    return true;
}
