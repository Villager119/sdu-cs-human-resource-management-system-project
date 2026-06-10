#ifndef SQLFILTERBUILDER_H
#define SQLFILTERBUILDER_H

#include <QString>
#include <QStringList>

namespace SqlFilter {

inline QString alwaysTrue()
{
    return "1=1";
}

inline QString alwaysFalse()
{
    return "1=0";
}

inline QString stringLiteral(QString value)
{
    value.replace("'", "''");
    return "'" + value + "'";
}

inline QString likePattern(QString value)
{
    value.replace("\\", "\\\\");
    value.replace("'", "''");
    value.replace("%", "\\%");
    value.replace("_", "\\_");
    return value;
}

inline QString equals(const QString &column, const QString &value)
{
    if (column.trimmed().isEmpty()) {
        return {};
    }
    return column + "=" + stringLiteral(value);
}

inline QString equalsInt(const QString &column, int value)
{
    if (column.trimmed().isEmpty()) {
        return {};
    }
    return QString("%1=%2").arg(column).arg(value);
}

inline QString contains(const QString &column, const QString &value)
{
    if (column.trimmed().isEmpty() || value.isEmpty()) {
        return {};
    }
    return column + " LIKE '%" + likePattern(value) + "%' ESCAPE '\\\\'";
}

inline QString containsAny(const QStringList &columns, const QString &value)
{
    QStringList conditions;
    for (const QString &column : columns) {
        const QString condition = contains(column, value);
        if (!condition.isEmpty()) {
            conditions << condition;
        }
    }
    if (conditions.isEmpty()) {
        return {};
    }
    if (conditions.size() == 1) {
        return conditions.first();
    }
    return "(" + conditions.join(" OR ") + ")";
}

inline QString inSubquery(const QString &column, const QString &subquery)
{
    if (column.trimmed().isEmpty() || subquery.trimmed().isEmpty()) {
        return {};
    }
    return QString("%1 IN (%2)").arg(column, subquery);
}

inline QString andAll(const QStringList &conditions, const QString &emptyFilter = {})
{
    QStringList nonEmpty;
    for (const QString &condition : conditions) {
        if (!condition.trimmed().isEmpty()) {
            nonEmpty << condition;
        }
    }
    return nonEmpty.isEmpty() ? emptyFilter : nonEmpty.join(" AND ");
}

}

#endif
