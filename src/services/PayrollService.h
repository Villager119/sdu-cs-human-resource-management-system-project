#ifndef PAYROLLSERVICE_H
#define PAYROLLSERVICE_H

#include <QList>
#include <QSqlDatabase>
#include <QString>

class PayrollService
{
public:
    struct Result {
        bool success = false;
        int generatedCount = 0;
        QString month;
        QString errorMessage;
    };

    explicit PayrollService(const QSqlDatabase &db = QSqlDatabase::database());

    bool payrollExists(const QString &month);
    Result calculateMonth(const QString &month, bool overwriteExisting);

private:
    struct TaxItem {
        QString name;
        double rate = 0.0;
    };

    struct ActiveEmployee {
        int empId = 0;
        double baseSalary = 0.0;
    };

    Result fail(const QString &month, const QString &message);
    double loadWorkDaysPerMonth();
    QList<TaxItem> loadTaxItems();
    double loadTaxThreshold();
    QList<ActiveEmployee> loadActiveEmployees();
    int leaveDaysForEmployee(int empId, const QString &month);
    double performanceBonusForEmployee(int empId, const QString &month, double baseSalary);
    bool deletePayrollRows(const QString &month, QString *errorText);
    bool insertPayrollRow(int empId, const QString &month, double baseSalary,
                          double leaveDeduction, double performanceBonus,
                          const QList<TaxItem> &taxItems, double taxThreshold,
                          const QString &issueDate, QString *errorText);

    QSqlDatabase m_db;
};

#endif
