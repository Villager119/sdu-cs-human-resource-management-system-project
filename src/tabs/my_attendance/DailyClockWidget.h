#ifndef DAILYCLOCKWIDGET_H
#define DAILYCLOCKWIDGET_H

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QTableView>
#include <QSqlRelationalTableModel>
#include <QTimer>

class DailyClockWidget : public QWidget
{
    Q_OBJECT
public:
    explicit DailyClockWidget(int empId, const QString &role, QWidget *parent = nullptr);
    void refresh();

signals:
    void logRequested(const QString &action, const QString &details);
    void notificationRequested(int empId, const QString &title, const QString &content);

private slots:
    void updateClock();
    void clockIn();
    void clockOut();

private:
    QWidget *createClockCard();
    QWidget *createAttendanceTable();
    void setupTimerAndConnections();
    void applyPermissionVisibility();

    int m_empId;
    QString m_role;

    QLabel *m_lblDate;
    QLabel *m_lblTime;
    QLabel *m_lblStatus;
    QPushButton *m_btnClockIn;
    QPushButton *m_btnClockOut;
    QTableView *m_attTable;
    QSqlRelationalTableModel *m_attModel;
    QTimer *m_clockTimer;
};

#endif // DAILYCLOCKWIDGET_H
