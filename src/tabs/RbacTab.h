#ifndef RBACTAB_H
#define RBACTAB_H

#include <QWidget>
#include <QListWidget>
#include <QCheckBox>
#include <QLineEdit>
#include <QPushButton>
#include <QMap>
#include <QSet>
#include <functional>
#include "../utils/UnsavedChangesGuard.h"

class RbacTab : public QWidget, public UnsavedChangesGuard
{
    Q_OBJECT
public:
    explicit RbacTab(std::function<void(const QString&, const QString&)> logFn = nullptr, QWidget *parent = nullptr);
    bool hasUnsavedChanges() const override;
    bool saveChanges() override;
    void discardChanges() override;
    QString unsavedChangesMessage() const override { return "角色权限还有未保存的修改，是否先保存再离开？"; }

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
    QSet<QString> m_loadedPermissionKeys;
    bool m_loadingPermissions = false;

    void updateRbacButtons();
    QSet<QString> checkedPermissionKeys() const;
    void logAction(const QString &action, const QString &target = QString());
};

#endif // RBACTAB_H
