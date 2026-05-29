#include "RbacTab.h"
#include "../core/SessionManager.h"
#include "../core/GlobalEvents.h"
#include "../services/AuditService.h"
#include "../services/RbacService.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QGroupBox>
#include <QScrollArea>
#include <QMessageBox>

RbacTab::RbacTab(std::function<void(const QString&, const QString&)> logFn, QWidget *parent)
    : QWidget(parent), m_log(logFn)
{
    QHBoxLayout *mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(10, 10, 10, 10);
    mainLayout->setSpacing(15);

    // Left Panel: Roles
    QGroupBox *roleBox = new QGroupBox("系统角色列表");
    QVBoxLayout *roleLayout = new QVBoxLayout(roleBox);
    roleLayout->setContentsMargins(10, 15, 10, 10);
    roleLayout->setSpacing(10);

    m_roleList = new QListWidget;
    m_roleList->setStyleSheet(
        "QListWidget { border: 1px solid #cbd5e1; border-radius: 6px; padding: 5px; }"
        "QListWidget::item { padding: 8px; border-radius: 4px; color: #334155; }"
        "QListWidget::item:hover { background-color: #f1f5f9; color: #0f172a; }"
        "QListWidget::item:selected { background-color: #eff6ff; color: #2563eb; font-weight: bold; }"
    );
    roleLayout->addWidget(m_roleList, 1);

    QHBoxLayout *addRoleLayout = new QHBoxLayout;
    m_newRoleEdit = new QLineEdit;
    m_newRoleEdit->setPlaceholderText("英文角色名称...");
    m_newRoleEdit->setMaxLength(30);

    m_btnAddRole = new QPushButton("添加角色");
    m_btnAddRole->setProperty("theme", "dark");

    m_btnDelRole = new QPushButton("删除");
    m_btnDelRole->setProperty("theme", "danger");
    m_btnDelRole->setEnabled(false);

    addRoleLayout->addWidget(m_newRoleEdit, 1);
    addRoleLayout->addWidget(m_btnAddRole);
    addRoleLayout->addWidget(m_btnDelRole);
    roleLayout->addLayout(addRoleLayout);

    // Right Panel: Permissions
    QGroupBox *permBox = new QGroupBox("角色权限配置");
    QVBoxLayout *permLayout = new QVBoxLayout(permBox);
    permLayout->setContentsMargins(10, 15, 10, 10);
    permLayout->setSpacing(10);

    QScrollArea *scrollArea = new QScrollArea;
    scrollArea->setWidgetResizable(true);
    scrollArea->setStyleSheet("QScrollArea { border: 1px solid #cbd5e1; border-radius: 6px; background-color: #ffffff; }");

    m_permsContainer = new QWidget;
    m_permsContainer->setObjectName("permsContainer");
    QVBoxLayout *permsContainerLayout = new QVBoxLayout(m_permsContainer);
    permsContainerLayout->setContentsMargins(15, 15, 15, 15);
    permsContainerLayout->setSpacing(12);

    scrollArea->setWidget(m_permsContainer);
    permLayout->addWidget(scrollArea, 1);

    QHBoxLayout *saveLayout = new QHBoxLayout;
    m_btnSavePerms = new QPushButton("保存权限修改");
    m_btnSavePerms->setEnabled(false);
    saveLayout->addStretch(1);
    saveLayout->addWidget(m_btnSavePerms);
    permLayout->addLayout(saveLayout);

    mainLayout->addWidget(roleBox, 3);
    mainLayout->addWidget(permBox, 5);

    // Connections
    connect(m_roleList, &QListWidget::currentItemChanged, this, &RbacTab::onRoleSelected);
    connect(m_btnAddRole, &QPushButton::clicked, this, &RbacTab::addRole);
    connect(m_btnDelRole, &QPushButton::clicked, this, &RbacTab::deleteRole);
    connect(m_btnSavePerms, &QPushButton::clicked, this, &RbacTab::savePermissions);

    // Initialize data
    loadPermissions();
    loadRoles();
}

void RbacTab::loadRoles()
{
    m_roleList->clear();
    const QVector<QString> roles = RbacService().roles();
    for (const QString &roleName : roles) {
        m_roleList->addItem(roleName);
    }

    if (m_roleList->count() > 0) {
        m_roleList->setCurrentRow(0);
    }
    updateRbacButtons();
}

void RbacTab::loadPermissions()
{
    // Clear layout
    QLayoutItem *child;
    while ((child = m_permsContainer->layout()->takeAt(0)) != nullptr) {
        if (child->widget()) {
            delete child->widget();
        }
        delete child;
    }
    m_permCheckBoxes.clear();

    QVBoxLayout *layout = qobject_cast<QVBoxLayout*>(m_permsContainer->layout());
    const QVector<RbacService::Permission> permissions = RbacService().permissions();
    for (const auto &permission : permissions) {
        QCheckBox *cb = new QCheckBox(QString("%1 (%2)").arg(permission.name, permission.key));
        cb->setStyleSheet(
            "QCheckBox { font-size: 13px; color: #334155; spacing: 8px; }"
            "QCheckBox::indicator { width: 18px; height: 18px; border-radius: 4px; border: 1px solid #cbd5e1; }"
            "QCheckBox::indicator:unchecked:hover { border-color: #3b82f6; background-color: #f8fafc; }"
            "QCheckBox::indicator:checked { border-color: #2563eb; background-color: #2563eb; image: url(check.png); /* Fallback to standard check drawing if no image */ }"
        );
        connect(cb, &QCheckBox::toggled, this, [this]() {
            if (!m_loadingPermissions) {
                updateRbacButtons();
            }
        });
        layout->addWidget(cb);
        m_permCheckBoxes.insert(permission.key, cb);
    }
    layout->addStretch(1);
}

void RbacTab::updateRbacButtons()
{
    QListWidgetItem *current = m_roleList->currentItem();
    if (!current) {
        m_btnDelRole->setEnabled(false);
        m_btnSavePerms->setEnabled(false);
        return;
    }

    QString roleName = current->text();
    bool isSystemRole = (roleName == "admin" || roleName == "user");
    m_btnDelRole->setEnabled(!isSystemRole);
    m_btnSavePerms->setEnabled(hasUnsavedChanges());
}

void RbacTab::onRoleSelected(QListWidgetItem *current, QListWidgetItem *previous)
{
    Q_UNUSED(previous);

    m_loadingPermissions = true;
    // Uncheck all first
    for (auto *cb : m_permCheckBoxes.values()) {
        cb->setChecked(false);
    }

    if (!current) {
        m_loadedPermissionKeys.clear();
        m_loadingPermissions = false;
        updateRbacButtons();
        return;
    }

    QString roleName = current->text();
    m_loadedPermissionKeys = RbacService().permissionKeysForRole(roleName);
    for (const QString &key : m_loadedPermissionKeys) {
        if (m_permCheckBoxes.contains(key)) {
            m_permCheckBoxes.value(key)->setChecked(true);
        }
    }
    m_loadingPermissions = false;

    updateRbacButtons();
}

QSet<QString> RbacTab::checkedPermissionKeys() const
{
    QSet<QString> checkedKeys;
    for (auto it = m_permCheckBoxes.begin(); it != m_permCheckBoxes.end(); ++it) {
        if (it.value()->isChecked()) {
            checkedKeys.insert(it.key());
        }
    }
    return checkedKeys;
}

bool RbacTab::hasUnsavedChanges() const
{
    return checkedPermissionKeys() != m_loadedPermissionKeys;
}

bool RbacTab::saveChanges()
{
    savePermissions();
    return !hasUnsavedChanges();
}

void RbacTab::discardChanges()
{
    onRoleSelected(m_roleList->currentItem(), nullptr);
}

void RbacTab::addRole()
{
    m_btnAddRole->setEnabled(false);
    m_btnDelRole->setEnabled(false);
    m_btnSavePerms->setEnabled(false);

    auto restoreButtons = [this]() {
        m_btnAddRole->setEnabled(true);
        updateRbacButtons();
    };

    QString roleName = m_newRoleEdit->text().trimmed();
    if (roleName.isEmpty()) {
        QMessageBox::warning(this, "添加失败", "角色名称不能为空！");
        restoreButtons();
        return;
    }

    const RbacService::Result result = RbacService().addRole(roleName);
    if (result.success) {
        logAction("添加角色", roleName);
        m_newRoleEdit->clear();
        loadRoles();
        
        // Find and select the newly added role
        for (int i = 0; i < m_roleList->count(); ++i) {
            if (m_roleList->item(i)->text() == roleName) {
                m_roleList->setCurrentRow(i);
                break;
            }
        }
        
        emit GlobalEvents::instance()->dataChanged();
        QMessageBox::information(this, "添加成功", QString("已成功添加角色 '%1'！").arg(roleName));
    } else {
        QMessageBox::critical(this, "添加失败", result.errorMessage);
    }

    restoreButtons();
}

void RbacTab::deleteRole()
{
    m_btnAddRole->setEnabled(false);
    m_btnDelRole->setEnabled(false);
    m_btnSavePerms->setEnabled(false);

    auto restoreButtons = [this]() {
        m_btnAddRole->setEnabled(true);
        updateRbacButtons();
    };

    QListWidgetItem *current = m_roleList->currentItem();
    if (!current) {
        restoreButtons();
        return;
    }

    QString roleName = current->text();
    auto reply = QMessageBox::question(this, "删除确认",
                                      QString("确定要删除角色 '%1' 吗？\n该角色下的所有员工将被自动降级为 'user' 角色。").arg(roleName),
                                      QMessageBox::Yes | QMessageBox::No);
    if (reply != QMessageBox::Yes) {
        restoreButtons();
        return;
    }

    const RbacService::Result result = RbacService().deleteRole(roleName);
    if (!result.success) {
        QMessageBox::critical(this, "删除失败", "删除角色失败！\n" + result.errorMessage);
        restoreButtons();
        return;
    }

    logAction("删除角色", roleName);
    loadRoles();

    emit GlobalEvents::instance()->dataChanged();
    QMessageBox::information(this, "删除成功", QString("已成功删除角色 '%1'！").arg(roleName));

    restoreButtons();
}

void RbacTab::savePermissions()
{
    m_btnAddRole->setEnabled(false);
    m_btnDelRole->setEnabled(false);
    m_btnSavePerms->setEnabled(false);

    auto restoreButtons = [this]() {
        m_btnAddRole->setEnabled(true);
        updateRbacButtons();
    };

    QListWidgetItem *current = m_roleList->currentItem();
    if (!current) {
        restoreButtons();
        return;
    }

    QString roleName = current->text();
    const int roleId = RbacService().roleId(roleName);
    QSet<QString> checkedKeys = checkedPermissionKeys();

    const RbacService::Result result = RbacService().saveRolePermissions(roleId, checkedKeys);
    if (!result.success) {
        QMessageBox::critical(this, "保存失败", result.errorMessage);
        restoreButtons();
        return;
    }

    // 3. If currently logged-in user shares this role, refresh session cached permissions immediately!
    if (SessionManager::instance()->role == roleName) {
        SessionManager::instance()->reloadPermissions();
    }

    logAction("修改角色权限", roleName);
    m_loadedPermissionKeys = checkedKeys;
    QMessageBox::information(this, "保存成功", QString("角色 '%1' 的权限配置已成功保存！").arg(roleName));

    restoreButtons();
}

void RbacTab::logAction(const QString &action, const QString &target)
{
    if (m_log) {
        m_log(action, target);
    } else {
        AuditService().writeLog(SessionManager::instance()->empId,
                                SessionManager::instance()->empName,
                                action,
                                target);
        emit GlobalEvents::instance()->auditRefresh();
    }
}
