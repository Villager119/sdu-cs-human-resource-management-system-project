#ifndef EMPLOYEESERVICE_H
#define EMPLOYEESERVICE_H

#include <QSqlDatabase>
#include <QDate>
#include <QString>
#include <QVariant>

class EmployeeService
{
public:
    struct EmployeeRecord {
        QString name;
        QString gender;
        QString phone;
        QString department;
        QString position;
        QString role;
        QString status;
        QString education;
        QString maritalStatus;
        QVariant baseSalary;
        QVariant hireDate;
        QVariant contractEndDate;
        QString title;
    };

    explicit EmployeeService(const QSqlDatabase &db = QSqlDatabase::database());

    QString defaultPasswordHash() const;
    QString toggledEmploymentStatus(const QString &currentStatus) const;
    bool validateEmployeeRecord(const EmployeeRecord &record, int displayRow, QString *errorMessage);
    bool departmentExists(const QString &department) const;
    bool roleExists(const QString &role) const;

private:
    QDate parseDate(const QVariant &value) const;

    QSqlDatabase m_db;
};

#endif
