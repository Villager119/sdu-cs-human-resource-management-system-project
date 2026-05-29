#ifndef ORGSERVICE_H
#define ORGSERVICE_H

#include <QMap>
#include <QSqlDatabase>
#include <QString>
#include <QVariant>
#include <QVector>

class OrgService
{
public:
    struct EmployeeOption {
        int employeeId = 0;
        QString name;
    };

    struct DepartmentNode {
        int departmentId = 0;
        QString name;
        int parentId = -1;
    };

    struct DepartmentDetail {
        bool found = false;
        QString name;
        int parentId = 0;
        int managerId = 0;
    };

    struct Result {
        bool success = false;
        QString errorMessage;
    };

    explicit OrgService(const QSqlDatabase &db = QSqlDatabase::database());

    QVector<EmployeeOption> activeEmployees() const;
    QMap<QString, int> activeEmployeeCountsByDepartment() const;
    int activeEmployeeCountInDepartment(const QString &departmentName) const;
    QVector<DepartmentNode> departments() const;
    DepartmentDetail departmentDetail(int departmentId) const;
    Result saveDepartment(int departmentId, const QString &name, const QVariant &parentId, const QVariant &managerId);
    Result assignEmployeeToDepartment(int employeeId, const QString &departmentName);
    Result removeDepartment(int departmentId);

private:
    Result fail(const QString &message) const;
    void bindNullableInt(QSqlQuery &query, const QVariant &value) const;
    bool wouldCreateParentCycle(int departmentId, int parentId, QString *errorText) const;
    int parentIdForDepartment(int departmentId, bool *ok) const;

    QSqlDatabase m_db;
};

#endif
