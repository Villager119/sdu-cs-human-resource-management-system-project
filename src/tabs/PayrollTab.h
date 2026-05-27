#ifndef PAYROLLTAB_H
#define PAYROLLTAB_H

#include <QWidget>
#include <QSqlRelationalTableModel>
#include <QTableView>
#include <QPushButton>
#include <QComboBox>
#include <QLabel>
#include <QList>
#include <functional>

class PayrollTab : public QWidget
{
    Q_OBJECT
public:
    PayrollTab(int empId, const QString &role,
               std::function<void(const QString&, const QString&)> logFn,
               QWidget *parent = nullptr);

public slots:
    void refresh();

private slots:
    void calculate();
    void exportCSV();
    void filterPayroll();

private:
    struct TaxItem {
        QString name;
        double rate = 0.0;
    };

    struct ActiveEmployee {
        int empId = 0;
        double baseSalary = 0.0;
    };

    void refreshMonthCombo();
    bool payrollExists(const QString &month);
    double loadWorkDaysPerMonth();
    QList<TaxItem> loadTaxItems();
    double loadTaxThreshold();
    QList<ActiveEmployee> loadActiveEmployees();
    int leaveDaysForEmployee(int empId, const QString &month);
    double performanceBonusForEmployee(int empId, const QString &month, double baseSalary);
    bool insertPayrollRow(int empId, const QString &month, double baseSalary,
                          double leaveDeduction, double performanceBonus,
                          const QList<TaxItem> &taxItems, double taxThreshold,
                          const QString &issueDate, QString *errorText);

    QSqlRelationalTableModel *m_model;
    QTableView *m_table;
    QPushButton *m_btnCalc;
    QComboBox *m_monthCombo;
    QPushButton *m_btnSearch;
    QPushButton *m_btnReset;

    int m_empId;
    QString m_role;
    std::function<void(const QString&, const QString&)> m_log;
};

#endif

