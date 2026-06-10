#include "OrgService.h"

#include "../core/Constants.h"
#include "../utils/DbQuery.h"

#include <QSqlError>
#include <QSqlQuery>
#include <QSet>

OrgService::OrgService(const QSqlDatabase &db)
    : m_db(db)
{
}

QVector<OrgService::EmployeeOption> OrgService::activeEmployees() const
{
    QVector<EmployeeOption> employees;
    QSqlQuery query(m_db);
    if (!DbQuery::execPrepared(query, "SELECT emp_id, name FROM employees WHERE status=?",
                               {HR::EmpStatus::ACTIVE})) {
        return employees;
    }
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
    if (!DbQuery::execPrepared(query,
                               "SELECT department, COUNT(*) FROM employees "
                               "WHERE department IS NOT NULL AND department!='' AND status=? "
                               "GROUP BY department",
                               {HR::EmpStatus::ACTIVE})) {
        return counts;
    }
    while (query.next()) {
        counts[query.value(0).toString()] = query.value(1).toInt();
    }
    query.finish();
    return counts;
}

int OrgService::activeEmployeeCountInDepartment(const QString &departmentName) const
{
    int count = 0;
    loadActiveEmployeeCountInDepartment(departmentName, &count, nullptr);
    return count;
}

QVector<OrgService::DepartmentNode> OrgService::departments() const
{
    QVector<DepartmentNode> nodes;
    QSqlQuery query(m_db);
    if (!DbQuery::exec(query, "SELECT dept_id, dept_name, parent_id FROM departments ORDER BY dept_id")) {
        return nodes;
    }
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
    loadDepartmentDetail(departmentId, &detail, nullptr);
    return detail;
}

OrgService::Result OrgService::saveDepartment(int departmentId, const QString &name,
                                              const QVariant &parentId, const QVariant &managerId)
{
    const QString trimmedName = name.trimmed();
    if (trimmedName.isEmpty()) {
        return fail("部门名称不能为空");
    }

    if (departmentId > 0) {
        QString detailError;
        DepartmentDetail existing;
        if (!loadDepartmentDetail(departmentId, &existing, &detailError)) {
            return fail("读取部门信息失败: " + detailError);
        }
        if (!existing.found) {
            return fail("未找到要保存的部门，请刷新组织架构后重试");
        }
    }

    const int newParentId = parentId.isValid() && !parentId.isNull() ? parentId.toInt() : 0;
    QString hierarchyError;
    if (wouldCreateParentCycle(departmentId, newParentId, &hierarchyError)) {
        return fail(hierarchyError);
    }

    QSqlQuery query(m_db);
    if (departmentId > 0) {
        query.prepare("UPDATE departments SET dept_name=?,parent_id=?,manager_id=? WHERE dept_id=?");
        query.addBindValue(trimmedName);
        bindNullableInt(query, parentId);
        bindNullableInt(query, managerId);
        query.addBindValue(departmentId);
    } else {
        query.prepare("INSERT INTO departments(dept_name,parent_id,manager_id) VALUES(?,?,?)");
        query.addBindValue(trimmedName);
        bindNullableInt(query, parentId);
        bindNullableInt(query, managerId);
    }

    QString errorText;
    const bool ok = DbQuery::execCurrent(query, &errorText);
    if (!ok) {
        return fail(errorText);
    }
    query.finish();

    Result result;
    result.success = true;
    return result;
}

OrgService::Result OrgService::assignEmployeeToDepartment(int employeeId, const QString &departmentName)
{
    if (employeeId <= 0) {
        return fail("请选择要调入部门的员工");
    }
    if (departmentName.trimmed().isEmpty()) {
        return fail("请选择目标部门");
    }

    QSqlQuery query(m_db);
    query.prepare("UPDATE employees SET department=?, version=version+1 WHERE emp_id=? AND status=?");
    query.addBindValue(departmentName.trimmed());
    query.addBindValue(employeeId);
    query.addBindValue(HR::EmpStatus::ACTIVE);

    QString errorText;
    const bool ok = DbQuery::execCurrent(query, &errorText);
    const int affected = query.numRowsAffected();
    if (!ok) {
        return fail(errorText);
    }
    query.finish();
    if (affected <= 0) {
        return fail("未找到在职员工，调入失败");
    }

    Result result;
    result.success = true;
    return result;
}

OrgService::Result OrgService::removeDepartment(int departmentId)
{
    QString errorText;
    DepartmentDetail detail;
    if (!loadDepartmentDetail(departmentId, &detail, &errorText)) {
        return fail("读取部门信息失败: " + errorText);
    }
    if (!detail.found) {
        return fail("未找到要删除的部门");
    }

    int employeeCount = 0;
    if (!loadActiveEmployeeCountInDepartment(detail.name, &employeeCount, &errorText)) {
        return fail("读取部门在职员工数量失败: " + errorText);
    }
    if (employeeCount > 0) {
        return fail(QString("该部门下仍有 %1 名在职员工，请先将员工调出后再删除").arg(employeeCount));
    }

    if (!m_db.transaction()) {
        return fail("启动部门删除事务失败: " + m_db.lastError().text());
    }

    if (!reparentChildDepartments(departmentId, &errorText)) {
        m_db.rollback();
        return fail(errorText);
    }

    if (!deleteDepartmentRow(departmentId, &errorText)) {
        m_db.rollback();
        return fail(errorText);
    }

    if (!m_db.commit()) {
        const QString commitErr = m_db.lastError().text();
        m_db.rollback();
        return fail("提交部门删除事务失败: " + commitErr);
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

bool OrgService::loadDepartmentDetail(int departmentId, DepartmentDetail *detail, QString *errorText) const
{
    if (detail) {
        *detail = DepartmentDetail();
    }

    QSqlQuery query(m_db);
    if (!DbQuery::execPrepared(query,
                               "SELECT dept_name, parent_id, manager_id FROM departments WHERE dept_id=?",
                               {departmentId}, errorText)) {
        return false;
    }
    if (query.next() && detail) {
        detail->found = true;
        detail->name = query.value(0).toString();
        detail->parentId = query.value(1).isNull() ? 0 : query.value(1).toInt();
        detail->managerId = query.value(2).isNull() ? 0 : query.value(2).toInt();
    }
    query.finish();
    return true;
}

bool OrgService::loadActiveEmployeeCountInDepartment(const QString &departmentName, int *count,
                                                     QString *errorText) const
{
    if (count) {
        *count = 0;
    }

    QSqlQuery query(m_db);
    if (!DbQuery::execPrepared(query,
                               "SELECT COUNT(*) FROM employees WHERE department=? AND status=?",
                               {departmentName, HR::EmpStatus::ACTIVE},
                               errorText)) {
        return false;
    }
    if (query.next() && count) {
        *count = query.value(0).toInt();
    }
    query.finish();
    return true;
}

bool OrgService::reparentChildDepartments(int departmentId, QString *errorText) const
{
    QSqlQuery query(m_db);
    return DbQuery::execPreparedAndFinish(query,
                                          "UPDATE departments SET parent_id=NULL WHERE parent_id=?",
                                          {departmentId}, errorText);
}

bool OrgService::deleteDepartmentRow(int departmentId, QString *errorText) const
{
    QSqlQuery query(m_db);
    if (!DbQuery::execPrepared(query, "DELETE FROM departments WHERE dept_id=?",
                               {departmentId}, errorText)) {
        return false;
    }
    const int affected = query.numRowsAffected();
    query.finish();
    if (affected != 1) {
        DbQuery::setError(errorText, "删除部门失败：目标部门不存在或状态已变化");
        return false;
    }
    return true;
}

bool OrgService::wouldCreateParentCycle(int departmentId, int parentId, QString *errorText) const
{
    if (departmentId <= 0) {
        return false;
    }
    if (parentId <= 0) {
        return false;
    }
    if (departmentId == parentId) {
        if (errorText) {
            *errorText = "上级部门不能设置为自身";
        }
        return true;
    }

    QSet<int> visited;
    visited.insert(departmentId);

    int current = parentId;
    while (current > 0) {
        if (current == departmentId) {
            if (errorText) {
                *errorText = "上级部门不能设置为当前部门的下级部门";
            }
            return true;
        }
        if (visited.contains(current)) {
            if (errorText) {
                *errorText = "部门层级关系已存在循环，请先修复组织架构数据";
            }
            return true;
        }
        visited.insert(current);

        bool ok = false;
        int nextParent = parentIdForDepartment(current, &ok);
        if (!ok) {
            if (errorText) {
                *errorText = "校验部门层级关系失败";
            }
            return true;
        }
        current = nextParent;
    }

    return false;
}

int OrgService::parentIdForDepartment(int departmentId, bool *ok) const
{
    if (ok) *ok = false;
    QSqlQuery query(m_db);

    int pid = 0;
    if (!DbQuery::execPrepared(query, "SELECT parent_id FROM departments WHERE dept_id = ?",
                               {departmentId})) {
        return pid;
    }
    if (query.next()) {
        if (ok) *ok = true;
        QVariant val = query.value(0);
        if (!val.isNull()) {
            pid = val.toInt();
        }
    }
    query.finish();
    return pid;
}
