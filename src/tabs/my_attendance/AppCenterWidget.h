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
    QDateEdit *m_leaveStart = nullptr;
    QDateEdit *m_leaveEnd = nullptr;
    QTextEdit *m_leaveReason = nullptr;
    QTableView *m_leaveTable = nullptr;
    QSqlRelationalTableModel *m_leaveModel = nullptr;

    // Makeup request components
    QDateEdit *m_makeupDate = nullptr;
    QComboBox *m_makeupType = nullptr;
    QTimeEdit *m_makeupTime = nullptr;
    QTextEdit *m_makeupReason = nullptr;
    QTableView *m_makeupTable = nullptr;
    QSqlRelationalTableModel *m_makeupModel = nullptr;
};

#endif // APPCENTERWIDGET_H
