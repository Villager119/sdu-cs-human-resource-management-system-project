#ifndef PERFORMANCETAB_H
#define PERFORMANCETAB_H

#include <QWidget>
#include <QSqlTableModel>
#include <QTableView>
#include <QComboBox>
#include <QSpinBox>
#include <QTextEdit>
#include <QLabel>
#include <functional>

#include <QLineEdit>
#include <QPushButton>
#include <QSqlRelationalTableModel>

class PerformanceTab : public QWidget
{
    Q_OBJECT
public:
    PerformanceTab(int empId, const QString &role,
                   std::function<void(const QString&, const QString&)> logFn,
                   QWidget *parent = nullptr);
    void refresh();

private slots:
    void submitScore();
    void updateTotal();
    void filterScores();
    void onAddScoreClicked();
    void onTableDoubleClicked(const QModelIndex &index);
    void closeDrawer();

private:
    QSqlRelationalTableModel *m_model;
    QTableView *m_table;

    // Filters Bar
    QComboBox *m_monthFilter;
    QLineEdit *m_nameFilter;
    QComboBox *m_deptFilter;
    QPushButton *m_btnAddScore;

    // Drawer components
    QWidget *m_scorePanel; // Represents the right-side drawer panel
    QLabel *m_drawerTitleLabel;
    QComboBox *m_empCombo;
    QLineEdit *m_drawerMonthEdit;
    QLabel *m_totalValLabel;
    QSpinBox *m_s1, *m_s2, *m_s3, *m_s4;
    QTextEdit *m_commentEdit;
    QPushButton *m_btnSubmit, *m_btnCancel, *m_btnCloseDrawer;

    int m_empId;
    QString m_role;
    std::function<void(const QString&, const QString&)> m_log;
};

#endif
