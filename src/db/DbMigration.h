#ifndef DBMIGRATION_H
#define DBMIGRATION_H

#include <QString>

bool ensureDbColumn(const QString &table, const QString &column, const QString &definition);
bool ensureDbIndex(const QString &table, const QString &indexName, const QString &columns);

#endif
