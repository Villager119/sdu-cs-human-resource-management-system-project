#ifndef EMPLOYEETAB_H
#define EMPLOYEETAB_H

#include <QWidget>
#include <QSqlTableModel>
#include <QTableView>
#include <QComboBox>
#include <QLineEdit>
#include <QPushButton>
#include <functional>

class PaginationBar;

class EmployeeTab : public QWidget
{
    Q_OBJECT
public:
    EmployeeTab(std::function<void(const QString&, const QString&)> logFn,
                QWidget *parent = nullptr);

private slots:
    void add();
    void remove();
    void save();
    void revert();
    void toggleStatus();
    void search();
    void resetFilter();
    void exportCSV();

private:
    QSqlTableModel *m_model;
    QTableView *m_table;
    QComboBox *m_deptCombo, *m_statusCombo;
    QLineEdit *m_nameSearch;
    QPushButton *m_btnToggleStatus;
    PaginationBar *m_pagination;
    std::function<void(const QString&, const QString&)> m_log;
};

#endif
