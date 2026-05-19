#ifndef SERVERSETTINGSDIALOG_H
#define SERVERSETTINGSDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QSpinBox>
#include <QListWidget>
#include <QProgressBar>
#include <QPushButton>

class ServerSettingsDialog : public QDialog
{
    Q_OBJECT
public:
    explicit ServerSettingsDialog(const QString &configPath, QWidget *parent = nullptr);

private slots:
    void onScan();
    void onTest();
    void onSave();

private:
    QStringList scanSubnet();

    QString m_configPath;

    QLineEdit *m_driverEdit;
    QLineEdit *m_serverEdit;
    QSpinBox  *m_portSpin;
    QLineEdit *m_dbEdit;
    QLineEdit *m_uidEdit;
    QLineEdit *m_pwdEdit;
    QListWidget *m_scanList;
    QProgressBar *m_progressBar;
    QPushButton *m_scanBtn;
    QPushButton *m_testBtn;
};

#endif
