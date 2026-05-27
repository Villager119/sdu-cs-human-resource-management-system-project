#include "OptimisticSqlTableModel.h"
#include <QSqlRecord>
#include <QSqlIndex>
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>

OptimisticSqlTableModel::OptimisticSqlTableModel(QObject *parent, const QSqlDatabase &db)
    : QSqlTableModel(parent, db)
{
}

bool OptimisticSqlTableModel::updateRowInTable(int row, const QSqlRecord &values)
{
    // Check if the table has a version column and a valid primary key
    QSqlIndex pk = primaryKey();
    int versionIdx = fieldIndex("version");

    if (pk.isEmpty() || versionIdx < 0) {
        // Fallback to default QSqlTableModel behavior if no optimistic locking version column or PK
        return QSqlTableModel::updateRowInTable(row, values);
    }

    // Retrieve the original record at the modified row to access primary key and version values
    QSqlRecord origRecord = record(row);
    
    // Construct target SQL query: UPDATE tableName SET col1 = ?, col2 = ?, version = version + 1 WHERE pkCol1 = ? AND version = ?
    QString tblName = tableName();
    QString updateSql = QString("UPDATE %1 SET ").arg(tblName);
    QStringList sets;
    
    // Determine which fields to update
    for (int i = 0; i < values.count(); ++i) {
        QString fName = values.fieldName(i);
        // Exclude primary key columns and the version column from standard set expressions
        if (pk.contains(fName) || fName.compare("version", Qt::CaseInsensitive) == 0) {
            continue;
        }
        if (values.isGenerated(i)) {
            sets.append(QString("%1 = :%1").arg(fName));
        }
    }
    
    if (sets.isEmpty()) {
        // No fields actually changed, so returning true is safe
        return true;
    }
    
    sets.append("version = version + 1");
    updateSql += sets.join(", ");
    
    // Add primary key and version checks to the WHERE clause
    QStringList wheres;
    for (int i = 0; i < pk.count(); ++i) {
        wheres.append(QString("%1 = :pk_%1").arg(pk.fieldName(i)));
    }
    wheres.append("version = :old_version");
    updateSql += " WHERE " + wheres.join(" AND ");

    QSqlQuery q(database());
    q.prepare(updateSql);
    
    // Bind updated fields
    for (int i = 0; i < values.count(); ++i) {
        QString fName = values.fieldName(i);
        if (pk.contains(fName) || fName.compare("version", Qt::CaseInsensitive) == 0) {
            continue;
        }
        if (values.isGenerated(i)) {
            q.bindValue(":" + fName, values.value(i));
        }
    }
    
    // Bind primary keys from original record
    for (int i = 0; i < pk.count(); ++i) {
        QString pkName = pk.fieldName(i);
        q.bindValue(":pk_" + pkName, origRecord.value(pkName));
    }
    
    // Bind old version
    int oldVersion = origRecord.value("version").toInt();
    q.bindValue(":old_version", oldVersion);
    
    if (!q.exec()) {
        qDebug() << "Optimistic SQL model save query error:" << q.lastError().text() << "SQL:" << updateSql;
        setLastError(q.lastError());
        q.finish();
        return false;
    }
    
    if (q.numRowsAffected() == 0) {
        // Concurrency conflict detected! No row matches the primary key and the old version
        qDebug() << "Optimistic concurrency collision on row" << row << "table" << tblName << "oldVersion" << oldVersion;
        QSqlError collisionError("OptimisticLockError", "数据已被其他用户修改，保存失败。请刷新重试！", QSqlError::UnknownError);
        setLastError(collisionError);
        q.finish();
        return false;
    }
    
    q.finish();
    return true;
}
