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
#include "../utils/UnsavedChangesGuard.h"

class QPushButton;
class OrgChartView;
class QTabWidget;

class OrgTab : public QWidget, public UnsavedChangesGuard
{
    Q_OBJECT
public:
    OrgTab(std::function<void(const QString&, const QString&)> logFn,
           QWidget *parent = nullptr);
    bool hasUnsavedChanges() const override;
    bool saveChanges() override;
    void discardChanges() override;
    QString unsavedChangesMessage() const override { return "组织信息还有未保存的修改，是否先保存再离开？"; }

public slots:
    void refresh();

private slots:
    void onTreeSelectionChanged();
    void saveDepartment();
    void removeDepartment();
    void assignEmployeeToSelectedDepartment();
    void addJobStandard();
    void removeJobStandard();
    void saveJobStandards();
    void updateJobStandardButtons();
    void markDepartmentDirty();

private:
    QTreeView *m_tree;
    QStandardItemModel *m_treeModel;
    QPushButton *m_btnDel;
    QPushButton *m_btnSave;
    QPushButton *m_btnAssignEmployee;
    QLineEdit *m_nameEdit;
    QComboBox *m_parentCombo, *m_managerCombo;
    int m_selectedDeptId = -1;
    std::function<void(const QString&, const QString&)> m_log;
    QTableView *m_empTable;
    QSqlTableModel *m_empModel;
    QTableView *m_jobTable;
    QSqlTableModel *m_jobModel;
    QPushButton *m_btnAddJob;
    QPushButton *m_btnDelJob;
    QPushButton *m_btnSaveJobs;
    QSplitter *m_splitter;
    OrgChartView *m_chartView;
    QTabWidget *m_tabWidget;
    bool m_loadingDepartment = false;
    bool m_departmentDirty = false;
};

#endif
