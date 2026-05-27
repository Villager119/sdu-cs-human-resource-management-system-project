#ifndef APPCENTERWIDGET_H
#define APPCENTERWIDGET_H

#include <QWidget>
#include <QDateEdit>
#include <QTimeEdit>
#include <QComboBox>
#include <QTextEdit>
#include <QTableView>
#include <QSqlRelationalTableModel>

class AppCenterWidget : public QWidget
{
    Q_OBJECT
public:
    explicit AppCenterWidget(int empId, const QString &role, QWidget *parent = nullptr);
    void refresh();

signals:
    void logRequested(const QString &action, const QString &details);
    void notificationRequested(int empId, const QString &title, const QString &content);

private slots:
    void submitLeave();
    void submitMakeup();

private:
    QWidget *createLeavePage(QTabWidget *tabs);
    QWidget *createMakeupPage(QTabWidget *tabs);

    int m_empId;
    QString m_role;

    // Leave request components
    QDateEdit *m_leaveStart;
    QDateEdit *m_leaveEnd;
    QTextEdit *m_leaveReason;
    QTableView *m_leaveTable;
    QSqlRelationalTableModel *m_leaveModel;

    // Makeup request components
    QDateEdit *m_makeupDate;
    QComboBox *m_makeupType;
    QTimeEdit *m_makeupTime;
    QTextEdit *m_makeupReason;
    QTableView *m_makeupTable;
    QSqlRelationalTableModel *m_makeupModel;
};

#endif // APPCENTERWIDGET_H
