#ifndef TAXCONFIGPANEL_H
#define TAXCONFIGPANEL_H

#include <QWidget>
#include <QTableView>
#include <QSqlTableModel>
#include <QPushButton>
#include <functional>

#include <QLineEdit>

class TaxConfigPanel : public QWidget
{
    Q_OBJECT
public:
    TaxConfigPanel(std::function<void(const QString&, const QString&)> logFn,
                   QWidget *parent = nullptr);

private slots:
    void save();

private:
    QSqlTableModel *m_model;
    QTableView *m_table;
    QLineEdit *m_workDaysEdit;
    QLineEdit *m_taxThresholdEdit;
    QPushButton *m_btnSave;
    std::function<void(const QString&, const QString&)> m_log;
};

#endif
