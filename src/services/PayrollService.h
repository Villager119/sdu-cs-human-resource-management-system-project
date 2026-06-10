#ifndef PAYROLLSERVICE_H
#define PAYROLLSERVICE_H

#include "PayrollCalculator.h"

#include <QList>
#include <QHash>
#include <QSqlDatabase>
#include <QString>

class QDate;

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

    bool payrollExists(const QString &month, QString *errorText = nullptr);
    Result calculateMonth(const QString &month, bool overwriteExisting);

private:
    struct ActiveEmployee {
        int empId = 0;
        double baseSalary = 0.0;
    };

    Result fail(const QString &month, const QString &message);
    bool loadWorkDaysPerMonth(double *workDays, QString *errorText);
    bool loadTaxItems(QList<PayrollCalc::TaxItem> *taxItems, QString *errorText);
    bool loadTaxThreshold(double *taxThreshold, QString *errorText);
    bool loadActiveEmployees(const QDate &monthStart, QList<ActiveEmployee> *employees, QString *errorText);
    bool loadLeaveDaysByEmployee(const QDate &monthStart, QHash<int, int> *leaveDaysByEmployee,
                                 QString *errorText);
    bool loadPerformanceScoresByEmployee(const QString &month, QHash<int, double> *scoresByEmployee,
                                         QString *errorText);
    bool deletePayrollRows(const QString &month, QString *errorText);
    bool insertPayrollRow(int empId, const QString &month, double baseSalary,
                          const PayrollCalc::Breakdown &breakdown,
                          const QString &issueDate, QString *errorText);

    QSqlDatabase m_db;
};

#endif
