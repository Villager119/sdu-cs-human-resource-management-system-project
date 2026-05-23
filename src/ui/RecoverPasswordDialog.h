#ifndef RECOVERPASSWORDDIALOG_H
#define RECOVERPASSWORDDIALOG_H

#include <QDialog>

class QLineEdit;
class QDateEdit;
class QStackedWidget;
class QLabel;
class QPushButton;

class RecoverPasswordDialog : public QDialog
{
    Q_OBJECT

public:
    explicit RecoverPasswordDialog(QWidget *parent = nullptr);
    ~RecoverPasswordDialog();

private slots:
    void onNextStep();
    void onResetPassword();
    void onBackStep();

private:
    void setupUi();

    QStackedWidget *m_stackedWidget;

    // Step 1: Verification
    QLineEdit *m_nameEdit;
    QLineEdit *m_phoneEdit;
    QLineEdit *m_empIdEdit;
    QLabel *m_errorLabelPage1;
    QPushButton *m_nextBtn;

    // Step 2: Reset Password
    QLineEdit *m_newPasswordEdit;
    QLineEdit *m_confirmPasswordEdit;
    QLabel *m_errorLabelPage2;
    QPushButton *m_resetBtn;
    QPushButton *m_backBtn;

    // Verified employee data
    int m_verifiedEmpId = -1;
    QString m_verifiedEmpName;
};

#endif // RECOVERPASSWORDDIALOG_H
