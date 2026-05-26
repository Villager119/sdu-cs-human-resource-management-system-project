#ifndef ATTENDBOARDWIDGET_H
#define ATTENDBOARDWIDGET_H

#include <QWidget>
#include <QDateEdit>
#include <QLineEdit>
#include <QComboBox>
#include <QTableView>
#include <QSqlRelationalTableModel>

class AttendBoardWidget : public QWidget
{
    Q_OBJECT
public:
    explicit AttendBoardWidget(int empId, const QString &role, QWidget *parent = nullptr);
    void refresh();

signals:
    void logRequested(const QString &action, const QString &details);

private slots:
    void filterAttendance();
    void resetFilters();
    void exportCsv();

private:
    int m_empId;
    QString m_role;

    QDateEdit *m_startDateFilter;
    QDateEdit *m_endDateFilter;
    QLineEdit *m_nameFilter;
    QComboBox *m_statusFilter;
    QTableView *m_attTable;
    QSqlRelationalTableModel *m_attModel;
};

#endif // ATTENDBOARDWIDGET_H
