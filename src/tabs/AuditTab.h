#ifndef AUDITTAB_H
#define AUDITTAB_H

#include <QWidget>
#include <QSqlTableModel>
#include <QTableView>
#include <QDateEdit>
#include <QLineEdit>

class PaginationBar;

class AuditTab : public QWidget
{
    Q_OBJECT
public:
    explicit AuditTab(QWidget *parent = nullptr);
    void refresh();
private:
    QString auditBaseFilter() const;
    QString auditPagedFilter(QString *errorText = nullptr) const;
    int filteredAuditCount(QString *errorText = nullptr) const;
    void loadAuditPage(bool resetPage = false);

    QSqlTableModel *m_model;
    QTableView *m_table;
    QLineEdit *m_keywordEdit;
    QDateEdit *m_startDateEdit;
    QDateEdit *m_endDateEdit;
    PaginationBar *m_pagination;
};

#endif
