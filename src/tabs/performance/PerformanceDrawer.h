#ifndef PERFORMANCEDRAWER_H
#define PERFORMANCEDRAWER_H

#include <QWidget>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
#include <QSpinBox>
#include <QTextEdit>
#include <QPushButton>

class PerformanceDrawer : public QWidget
{
    Q_OBJECT
public:
    explicit PerformanceDrawer(QWidget *parent = nullptr);
    void setupAddMode();
    void setupEditMode(int empId, const QString &month, int a1, int a2, int a3, int a4, const QString &comment);

signals:
    void logRequested(const QString &action, const QString &details);
    void scoreSubmitted();
    void closeRequested();

private slots:
    void submitScore();
    void updateTotal();

private:
    QLabel *m_drawerTitleLabel;
    QComboBox *m_empCombo;
    QLineEdit *m_drawerMonthEdit;
    QLabel *m_totalValLabel;
    QSpinBox *m_s1;
    QSpinBox *m_s2;
    QSpinBox *m_s3;
    QSpinBox *m_s4;
    QTextEdit *m_commentEdit;

    QPushButton *m_btnSubmit;
    QPushButton *m_btnCancel;
    QPushButton *m_btnCloseDrawer;

    bool m_isEditMode;
    void loadEmployees();
};

#endif // PERFORMANCEDRAWER_H
