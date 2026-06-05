#ifndef EMPLOYEETAB_H
#define EMPLOYEETAB_H

#include <QWidget>
#include "../widgets/OptimisticSqlTableModel.h"
#include <QTableView>
#include <QComboBox>
#include <QLineEdit>
#include <QPushButton>
#include <functional>
#include "../utils/UnsavedChangesGuard.h"

class PaginationBar;
class QVBoxLayout;

class EmployeeTab : public QWidget, public UnsavedChangesGuard
{
    Q_OBJECT
public:
    EmployeeTab(std::function<void(const QString&, const QString&)> logFn,
                QWidget *parent = nullptr);
    bool hasUnsavedChanges() const override;
    bool saveChanges() override;
    void discardChanges() override { revertChanges(); }
    QString unsavedChangesMessage() const override { return "员工信息还有未保存的修改，是否先保存再离开？"; }
    void revertChanges();

public slots:
    void refresh();

private slots:
    void add();
    void remove();
    void save();
    void revert();
    void toggleStatus();
    void search();
    void resetFilter();
    void batchChangeDept();
    void exportCSV();
    void updateDirtyState();

private:
    void importExistingDepartments();
    void setupModel();
    void setupDelegates();
    void setupFilterBar(QVBoxLayout *layout);
    void setupPagination(QVBoxLayout *layout);
    void setupActionBar(QVBoxLayout *layout);
    void setupModelSignals();
    QStringList loadDepartments() const;
    QStringList loadRoles() const;
    QStringList loadShiftIds() const;
    QStringList loadPositionsForDepartment(const QString &department) const;
    QStringList loadTitlesForJob(const QString &department, const QString &position) const;
    bool defaultSalaryForJob(const QString &department, const QString &position, const QString &title, double *salary) const;
    void applyJobStandardForRow(int row, int changedColumn);
    bool validateRows();
    bool validateEmployeeRow(int row);
    QString employeeBaseFilter() const;
    QString employeePagedFilter(QString *errorText = nullptr) const;
    int filteredEmployeeCount(QString *errorText = nullptr) const;
    void loadEmployeePage(bool resetPage = false);
    bool handlePendingChangesBeforePageSwitch();

    OptimisticSqlTableModel *m_model;
    QTableView *m_table;
    QComboBox *m_deptCombo, *m_statusCombo, *m_maritalCombo, *m_eduCombo;
    QLineEdit *m_nameSearch, *m_posSearch;
    QPushButton *m_btnToggleStatus;
    QPushButton *m_btnDel;
    QPushButton *m_btnRevert;
    QPushButton *m_btnSave;
    QPushButton *m_btnBatchDept;
    PaginationBar *m_pagination;
    std::function<void(const QString&, const QString&)> m_log;
    bool m_syncingJobFields = false;
    int m_loadedEmployeePage = 1;

    // Dynamic column indexes
    int m_idxEmpId = -1;
    int m_idxName = -1;
    int m_idxGender = -1;
    int m_idxPhone = -1;
    int m_idxDept = -1;
    int m_idxRole = -1;
    int m_idxPwd = -1;
    int m_idxSalary = -1;
    int m_idxHire = -1;
    int m_idxContract = -1;
    int m_idxShift = -1;
    int m_idxStatus = -1;
    int m_idxEdu = -1;
    int m_idxMarital = -1;
    int m_idxPos = -1;
    int m_idxTitle = -1;
};

#endif
