#ifndef AUDITTAB_H
#define AUDITTAB_H

#include <QWidget>
#include <QSqlTableModel>
#include <QTableView>

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
