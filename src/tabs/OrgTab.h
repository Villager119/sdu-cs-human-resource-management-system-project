#ifndef ORGTAB_H
#define ORGTAB_H

#include <QWidget>
#include <QTreeView>
#include <QTableView>
#include <QStandardItemModel>
#include <QSqlTableModel>
#include <QSplitter>
#include <QLineEdit>
#include <QComboBox>
#include <functional>

class QPushButton;
class OrgChartView;
class QTabWidget;

class OrgTab : public QWidget
{
    Q_OBJECT
public:
    OrgTab(std::function<void(const QString&, const QString&)> logFn,
           QWidget *parent = nullptr);

public slots:
    void refresh();

private slots:
    void onTreeSelectionChanged();
    void saveDepartment();
    void removeDepartment();

private:
    QTreeView *m_tree;
    QStandardItemModel *m_treeModel;
    QPushButton *m_btnDel;
    QLineEdit *m_nameEdit;
    QComboBox *m_parentCombo, *m_managerCombo;
    int m_selectedDeptId = -1;
    std::function<void(const QString&, const QString&)> m_log;
    QTableView *m_empTable;
    QSqlTableModel *m_empModel;
    QSplitter *m_splitter;
    OrgChartView *m_chartView;
    QTabWidget *m_tabWidget;
};

#endif
