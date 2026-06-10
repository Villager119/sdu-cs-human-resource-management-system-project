#ifndef PAYROLLCALCULATOR_H
#define PAYROLLCALCULATOR_H

#include <QDate>
#include <QList>
#include <QString>

#include <algorithm>

namespace PayrollCalc {

constexpr double DefaultWorkDaysPerMonth = 21.75;
constexpr double DefaultTaxThreshold = 5000.0;

struct TaxItem {
    QString name;
    double rate = 0.0;
};

struct Input {
    double baseSalary = 0.0;
    int leaveDays = 0;
    double performanceScore = 0.0;
    double workDaysPerMonth = DefaultWorkDaysPerMonth;
    double taxThreshold = DefaultTaxThreshold;
    QList<TaxItem> taxItems;
};

struct Breakdown {
    double leaveDeduction = 0.0;
    double performanceBonus = 0.0;
    double pension = 0.0;
    double medical = 0.0;
    double unemployment = 0.0;
    double housing = 0.0;
    double incomeTax = 0.0;
    double netSalary = 0.0;
};

inline QString invalidMonthMessage()
{
    return "薪资月份格式无效，应为 yyyy-MM";
}

inline bool parseMonth(const QString &month, QDate *monthStart, QString *errorText = nullptr)
{
    const QDate parsed = QDate::fromString(month + "-01", "yyyy-MM-dd");
    if (!parsed.isValid()) {
        if (errorText) {
            *errorText = invalidMonthMessage();
        }
        return false;
    }
    if (monthStart) {
        *monthStart = parsed;
    }
    return true;
}

inline bool isWorkday(const QDate &date)
{
    return date.isValid() && date.dayOfWeek() >= 1 && date.dayOfWeek() <= 5;
}

inline double personalIncomeTax(double taxable)
{
    if (taxable > 25000) return taxable * 0.25 - 2660;
    if (taxable > 12000) return taxable * 0.20 - 1410;
    if (taxable > 3000) return taxable * 0.10 - 210;
    if (taxable > 0) return taxable * 0.03;
    return 0.0;
}

inline double bonusForScore(double baseSalary, double score)
{
    if (score >= 90) return baseSalary * 0.10;
    if (score >= 70) return baseSalary * 0.05;
    return 0.0;
}

inline Breakdown calculate(const Input &input)
{
    Breakdown result;

    const double workDays = input.workDaysPerMonth > 0
        ? input.workDaysPerMonth
        : DefaultWorkDaysPerMonth;
    result.leaveDeduction = std::min((input.baseSalary / workDays) * input.leaveDays,
                                     input.baseSalary);
    result.performanceBonus = bonusForScore(input.baseSalary, input.performanceScore);

    for (const TaxItem &item : input.taxItems) {
        const double value = input.baseSalary * item.rate;
        if (item.name == "养老保险") result.pension = value;
        else if (item.name == "医疗保险") result.medical = value;
        else if (item.name == "失业保险") result.unemployment = value;
        else if (item.name == "住房公积金") result.housing = value;
    }

    const double taxable = input.baseSalary - result.leaveDeduction + result.performanceBonus
                           - result.pension - result.medical - result.unemployment
                           - result.housing - input.taxThreshold;
    result.incomeTax = std::max(0.0, personalIncomeTax(taxable));

    result.netSalary = std::max(0.0, input.baseSalary - result.leaveDeduction
                                      + result.performanceBonus - result.pension
                                      - result.medical - result.unemployment
                                      - result.housing - result.incomeTax);
    return result;
}

}

#endif
