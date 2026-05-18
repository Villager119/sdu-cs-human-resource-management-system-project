#ifndef PAYROLLTAB_H
#define PAYROLLTAB_H

#include <QWidget>
#include <QSqlRelationalTableModel>
#include <QTableView>
#include <QPushButton>
#include <functional>

class PayrollTab : public QWidget
{
    Q_OBJECT
public:
    PayrollTab(int empId, const QString &role,
               std::function<void(const QString&, const QString&)> logFn,
               QWidget *parent = nullptr);

private slots:
    void calculate();
    void exportCSV();

private:
    QSqlRelationalTableModel *m_model;
    QTableView *m_table;
    QPushButton *m_btnCalc;
    int m_empId;
    QString m_role;
    std::function<void(const QString&, const QString&)> m_log;
};

#endif
