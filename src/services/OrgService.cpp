#include "OrgService.h"

#include <QSqlError>
#include <QSqlQuery>

OrgService::OrgService(const QSqlDatabase &db)
    : m_db(db)
{
}

QVector<OrgService::EmployeeOption> OrgService::activeEmployees() const
{
    QVector<EmployeeOption> employees;
    QSqlQuery query(m_db);
    query.exec("SELECT emp_id, name FROM employees WHERE status='在职'");
    while (query.next()) {
        employees.append({query.value(0).toInt(), query.value(1).toString()});
    }
    query.finish();
    return employees;
}

QMap<QString, int> OrgService::activeEmployeeCountsByDepartment() const
{
    QMap<QString, int> counts;
    QSqlQuery query(m_db);
    query.exec("SELECT department, COUNT(*) FROM employees "
               "WHERE department IS NOT NULL AND department!='' AND status='在职' "
               "GROUP BY department");
    while (query.next()) {
        counts[query.value(0).toString()] = query.value(1).toInt();
    }
    query.finish();
    return counts;
}

QVector<OrgService::DepartmentNode> OrgService::departments() const
{
    QVector<DepartmentNode> nodes;
    QSqlQuery query(m_db);
    query.exec("SELECT dept_id, dept_name, parent_id FROM departments ORDER BY dept_id");
    while (query.next()) {
        DepartmentNode node;
        node.departmentId = query.value(0).toInt();
        node.name = query.value(1).toString();
        node.parentId = query.value(2).isNull() ? -1 : query.value(2).toInt();
        nodes.append(node);
    }
    query.finish();
    return nodes;
}

OrgService::DepartmentDetail OrgService::departmentDetail(int departmentId) const
{
    DepartmentDetail detail;
    QSqlQuery query(m_db);
    query.prepare("SELECT dept_name, parent_id, manager_id FROM departments WHERE dept_id=?");
    query.addBindValue(departmentId);
    if (query.exec() && query.next()) {
        detail.found = true;
        detail.name = query.value(0).toString();
        detail.parentId = query.value(1).isNull() ? 0 : query.value(1).toInt();
        detail.managerId = query.value(2).isNull() ? 0 : query.value(2).toInt();
    }
    query.finish();
    return detail;
}

OrgService::Result OrgService::saveDepartment(int departmentId, const QString &name,
                                              const QVariant &parentId, const QVariant &managerId)
{
    QSqlQuery query(m_db);
    if (departmentId > 0) {
        query.prepare("UPDATE departments SET dept_name=?,parent_id=?,manager_id=? WHERE dept_id=?");
        query.addBindValue(name);
        bindNullableInt(query, parentId);
        bindNullableInt(query, managerId);
        query.addBindValue(departmentId);
    } else {
        query.prepare("INSERT IGNORE INTO departments(dept_name,parent_id,manager_id) VALUES(?,?,?)");
        query.addBindValue(name);
        bindNullableInt(query, parentId);
        bindNullableInt(query, managerId);
    }

    const bool ok = query.exec();
    const QString errorText = query.lastError().text();
    query.finish();
    if (!ok) {
        return fail(errorText);
    }

    Result result;
    result.success = true;
    return result;
}

OrgService::Result OrgService::removeDepartment(int departmentId)
{
    if (!m_db.transaction()) {
        return fail("启动部门删除事务失败: " + m_db.lastError().text());
    }

    QSqlQuery updateChildren(m_db);
    updateChildren.prepare("UPDATE departments SET parent_id=NULL WHERE parent_id=?");
    updateChildren.addBindValue(departmentId);
    if (!updateChildren.exec()) {
        const QString errorText = updateChildren.lastError().text();
        updateChildren.finish();
        m_db.rollback();
        return fail(errorText);
    }
    updateChildren.finish();

    QSqlQuery deleteDepartment(m_db);
    deleteDepartment.prepare("DELETE FROM departments WHERE dept_id=?");
    deleteDepartment.addBindValue(departmentId);
    if (!deleteDepartment.exec()) {
        const QString errorText = deleteDepartment.lastError().text();
        deleteDepartment.finish();
        m_db.rollback();
        return fail(errorText);
    }
    deleteDepartment.finish();

    if (!m_db.commit()) {
        return fail("提交部门删除事务失败: " + m_db.lastError().text());
    }

    Result result;
    result.success = true;
    return result;
}

OrgService::Result OrgService::fail(const QString &message) const
{
    Result result;
    result.errorMessage = message;
    return result;
}

void OrgService::bindNullableInt(QSqlQuery &query, const QVariant &value) const
{
    if (value.isNull()) {
        query.addBindValue(QVariant(QMetaType(QMetaType::Int)));
    } else {
        query.addBindValue(value.toInt());
    }
}
