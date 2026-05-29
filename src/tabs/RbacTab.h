#ifndef RBACTAB_H
#define RBACTAB_H

#include <QWidget>
#include <QListWidget>
#include <QCheckBox>
#include <QLineEdit>
#include <QPushButton>
#include <QMap>
#include <functional>

class RbacTab : public QWidget
{
    Q_OBJECT
public:
    explicit RbacTab(std::function<void(const QString&, const QString&)> logFn = nullptr, QWidget *parent = nullptr);

private slots:
    void loadRoles();
    void loadPermissions();
    void onRoleSelected(QListWidgetItem *current, QListWidgetItem *previous);
    void addRole();
    void deleteRole();
    void savePermissions();

private:
    std::function<void(const QString&, const QString&)> m_log;
    QListWidget *m_roleList;
    QLineEdit *m_newRoleEdit;
    QPushButton *m_btnAddRole;
    QPushButton *m_btnDelRole;
    QPushButton *m_btnSavePerms;
    QWidget *m_permsContainer;
    QMap<QString, QCheckBox*> m_permCheckBoxes;

    void updateRbacButtons();
    void logAction(const QString &action, const QString &target = QString());
};

#endif // RBACTAB_H
