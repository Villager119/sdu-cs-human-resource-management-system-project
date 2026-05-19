#ifndef PERFORMANCETAB_H
#define PERFORMANCETAB_H

#include <QWidget>
#include <QSqlTableModel>
#include <QTableView>
#include <QComboBox>
#include <QSpinBox>
#include <QTextEdit>
#include <functional>

class PerformanceTab : public QWidget
{
    Q_OBJECT
public:
    PerformanceTab(int empId, const QString &role,
                   std::function<void(const QString&, const QString&)> logFn,
                   QWidget *parent = nullptr);

private slots:
    void submitScore();
    void refresh();

private:
    QSqlTableModel *m_model;
    QTableView *m_table;
    QComboBox *m_empCombo, *m_monthCombo;
    QSpinBox *m_scoreSpin;
    QTextEdit *m_commentEdit;
    QWidget *m_scorePanel;
    int m_empId;
    QString m_role;
    std::function<void(const QString&, const QString&)> m_log;
};

#endif
