#ifndef PERFORMANCEDRAWER_H
#define PERFORMANCEDRAWER_H

#include <QWidget>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
#include <QSpinBox>
#include <QTextEdit>
#include <QPushButton>
#include "../../utils/UnsavedChangesGuard.h"

class PerformanceDrawer : public QWidget, public UnsavedChangesGuard
{
    Q_OBJECT
public:
    explicit PerformanceDrawer(QWidget *parent = nullptr);
    void setupAddMode();
    void setupEditMode(int empId, const QString &month, int a1, int a2, int a3, int a4, const QString &comment);
    bool hasUnsavedChanges() const override;
    bool saveChanges() override;
    void discardChanges() override;
    QString unsavedChangesMessage() const override { return "绩效评分还有未提交的修改，是否先提交再离开？"; }

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
    int m_savedEmpId = 0;
    QString m_savedMonth;
    int m_savedS1 = 0;
    int m_savedS2 = 0;
    int m_savedS3 = 0;
    int m_savedS4 = 0;
    QString m_savedComment;
    void loadEmployees();
    void rememberCurrentState();
};

#endif // PERFORMANCEDRAWER_H
