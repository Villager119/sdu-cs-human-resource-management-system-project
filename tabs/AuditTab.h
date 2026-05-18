#ifndef AUDITTAB_H
#define AUDITTAB_H

#include <QWidget>
#include <QSqlTableModel>
#include <QVBoxLayout>
#include <QTableView>
#include <QSqlQuery>

class AuditTab : public QWidget
{
    Q_OBJECT
public:
    explicit AuditTab(QWidget *parent = nullptr);
    void refresh();
private:
    QSqlTableModel *m_model;
    QTableView *m_table;
};

#endif
