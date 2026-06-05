#ifndef EMPLOYEESERVICE_H
#define EMPLOYEESERVICE_H

#include <QSqlDatabase>
#include <QDate>
#include <QString>
#include <QVariant>

class EmployeeService
{
public:
    struct JobSalaryStandard {
        bool found = false;
        double minSalary = 0.0;
        double maxSalary = 0.0;
        double defaultSalary = 0.0;
    };

    struct EmployeeRecord {
        int employeeId = 0;
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
        QVariant shiftId;
        QString title;
    };

    explicit EmployeeService(const QSqlDatabase &db = QSqlDatabase::database());

    QString defaultPasswordHash() const;
    QString toggledEmploymentStatus(const QString &currentStatus) const;
    bool validateEmployeeRecord(const EmployeeRecord &record, int displayRow, QString *errorMessage);
    bool departmentExists(const QString &department) const;
    bool roleExists(const QString &role) const;
    bool shiftExists(int shiftId) const;
    bool phoneAvailableForEmployee(const QString &phone, int employeeId, QString *errorMessage = nullptr) const;
    JobSalaryStandard jobSalaryStandard(const QString &department, const QString &position, const QString &title) const;

private:
    bool isValidEmployeeName(const QString &name) const;
    QDate parseDate(const QVariant &value) const;

    QSqlDatabase m_db;
};

#endif
