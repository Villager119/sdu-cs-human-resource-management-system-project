#ifndef PERFORMANCETAB_H
#define PERFORMANCETAB_H

#include <QWidget>
#include <QTableView>
#include <QSqlRelationalTableModel>
#include <QComboBox>
#include <QLineEdit>
#include <QPushButton>
#include <functional>

class PerformanceDrawer;

class PerformanceTab : public QWidget
{
    Q_OBJECT
public:
    PerformanceTab(int empId, const QString &role,
                   std::function<void(const QString&, const QString&)> logFn,
                   QWidget *parent = nullptr);
    void refresh();

private slots:
    void filterScores();
    void onAddScoreClicked();
    void onTableDoubleClicked(const QModelIndex &index);
    void closeDrawer();

private:
    QSqlRelationalTableModel *m_model;
    QTableView *m_table;

    // Filters Bar
    QComboBox *m_monthFilter;
    QLineEdit *m_nameFilter;
    QComboBox *m_deptFilter;
    QPushButton *m_btnAddScore;

    // Slide Drawer
    PerformanceDrawer *m_drawer;

    int m_empId;
    QString m_role;
    std::function<void(const QString&, const QString&)> m_log;
};

#endif // PERFORMANCETAB_H
