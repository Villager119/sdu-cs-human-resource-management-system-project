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
    QSqlTableModel *m_model;
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
    int m_idxStatus = -1;
    int m_idxEdu = -1;
    int m_idxMarital = -1;
    int m_idxPos = -1;
    int m_idxTitle = -1;
};

#endif
