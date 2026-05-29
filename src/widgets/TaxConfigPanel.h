#ifndef TAXCONFIGPANEL_H
#define TAXCONFIGPANEL_H

#include <QWidget>
#include <QTableView>
#include <QSqlTableModel>
#include <QPushButton>
#include <functional>

#include <QLineEdit>
#include "../utils/UnsavedChangesGuard.h"

class TaxConfigPanel : public QWidget, public UnsavedChangesGuard
{
    Q_OBJECT
public:
    TaxConfigPanel(std::function<void(const QString&, const QString&)> logFn,
                   QWidget *parent = nullptr);
    bool hasUnsavedChanges() const override;
    bool saveChanges() override;
    void discardChanges() override;
    QString unsavedChangesMessage() const override { return "薪资及社保配置还有未保存的修改，是否先保存再离开？"; }

private slots:
    void save();

private:
    void loadSettings();
    bool saveInternal(bool showMessage);

    QSqlTableModel *m_model;
    QTableView *m_table;
    QLineEdit *m_workDaysEdit;
    QLineEdit *m_taxThresholdEdit;
    QPushButton *m_btnSave;
    std::function<void(const QString&, const QString&)> m_log;
    QString m_savedWorkDays;
    QString m_savedTaxThreshold;
};

#endif
