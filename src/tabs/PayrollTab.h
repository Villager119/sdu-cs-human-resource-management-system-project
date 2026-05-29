#ifndef PAYROLLTAB_H
#define PAYROLLTAB_H

#include <QWidget>
#include <QSqlRelationalTableModel>
#include <QTableView>
#include <QPushButton>
#include <QComboBox>
#include <QLabel>
#include <QLineEdit>
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
    void refreshMonthCombo();

    QSqlRelationalTableModel *m_model;
    QTableView *m_table;
    QPushButton *m_btnCalc;
    QComboBox *m_monthCombo;
    QLineEdit *m_employeeNameEdit;
    QPushButton *m_btnSearch;
    QPushButton *m_btnReset;

    int m_empId;
    QString m_role;
    std::function<void(const QString&, const QString&)> m_log;
};

#endif

