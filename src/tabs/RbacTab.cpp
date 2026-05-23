#include "RbacTab.h"
#include "../core/SessionManager.h"
#include "../core/GlobalEvents.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QGroupBox>
#include <QScrollArea>
#include <QMessageBox>
#include <QSqlQuery>
#include <QSqlError>
#include <QSqlDatabase>
#include <QRegularExpression>
#include <QDebug>

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
    QSqlQuery q;
    if (q.exec("SELECT role_name FROM roles ORDER BY role_id")) {
        while (q.next()) {
            m_roleList->addItem(q.value(0).toString());
        }
    } else {
        qDebug() << "Failed to load roles:" << q.lastError().text();
    }

    if (m_roleList->count() > 0) {
        m_roleList->setCurrentRow(0);
    }
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

    QSqlQuery q;
    if (q.exec("SELECT permission_key, permission_name FROM permissions ORDER BY permission_id")) {
        QVBoxLayout *layout = qobject_cast<QVBoxLayout*>(m_permsContainer->layout());
        while (q.next()) {
            QString key = q.value(0).toString();
            QString name = q.value(1).toString();

            QCheckBox *cb = new QCheckBox(QString("%1 (%2)").arg(name, key));
            cb->setStyleSheet(
                "QCheckBox { font-size: 13px; color: #334155; spacing: 8px; }"
                "QCheckBox::indicator { width: 18px; height: 18px; border-radius: 4px; border: 1px solid #cbd5e1; }"
                "QCheckBox::indicator:unchecked:hover { border-color: #3b82f6; background-color: #f8fafc; }"
                "QCheckBox::indicator:checked { border-color: #2563eb; background-color: #2563eb; image: url(check.png); /* Fallback to standard check drawing if no image */ }"
            );
            layout->addWidget(cb);
            m_permCheckBoxes.insert(key, cb);
        }
        layout->addStretch(1);
    } else {
        qDebug() << "Failed to load permissions:" << q.lastError().text();
    }
}

void RbacTab::onRoleSelected(QListWidgetItem *current, QListWidgetItem *previous)
{
    Q_UNUSED(previous);

    // Uncheck all first
    for (auto *cb : m_permCheckBoxes.values()) {
        cb->setChecked(false);
    }

    if (!current) {
        m_btnDelRole->setEnabled(false);
        m_btnSavePerms->setEnabled(false);
        return;
    }

    m_btnSavePerms->setEnabled(true);
    QString roleName = current->text();

    // Disable deletion of system roles
    bool isSystemRole = (roleName == "admin" || roleName == "user");
    m_btnDelRole->setEnabled(!isSystemRole);

    // Load active permissions for this role
    QSqlQuery q;
    q.prepare("SELECT p.permission_key FROM permissions p "
              "JOIN role_permissions rp ON p.permission_id = rp.permission_id "
              "JOIN roles r ON rp.role_id = r.role_id "
              "WHERE r.role_name = ?");
    q.addBindValue(roleName);
    if (q.exec()) {
        while (q.next()) {
            QString key = q.value(0).toString();
            if (m_permCheckBoxes.contains(key)) {
                m_permCheckBoxes.value(key)->setChecked(true);
            }
        }
    } else {
        qDebug() << "Failed to load permissions for role:" << roleName << q.lastError().text();
    }
}

void RbacTab::addRole()
{
    QString roleName = m_newRoleEdit->text().trimmed();
    if (roleName.isEmpty()) {
        QMessageBox::warning(this, "提示", "角色名称不能为空！");
        return;
    }

    // Alphanumeric check
    QRegularExpression re("^[a-zA-Z0-9_]+$");
    if (!re.match(roleName).hasMatch()) {
        QMessageBox::warning(this, "提示", "角色名称只能包含字母、数字和下划线！");
        return;
    }

    QSqlQuery q;
    q.prepare("INSERT INTO roles (role_name) VALUES (?)");
    q.addBindValue(roleName);
    if (q.exec()) {
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
        QMessageBox::information(this, "成功", QString("已成功添加角色 '%1'！").arg(roleName));
    } else {
        QMessageBox::critical(this, "错误", "无法添加角色，可能已存在同名角色！\n" + q.lastError().text());
    }
}

void RbacTab::deleteRole()
{
    QListWidgetItem *current = m_roleList->currentItem();
    if (!current) return;

    QString roleName = current->text();
    if (roleName == "admin" || roleName == "user") {
        QMessageBox::warning(this, "提示", "系统默认角色无法删除！");
        return;
    }

    auto reply = QMessageBox::question(this, "确认删除",
                                      QString("确定要删除角色 '%1' 吗？\n该角色下的所有员工将被自动降级为 'user' 角色。").arg(roleName),
                                      QMessageBox::Yes | QMessageBox::No);
    if (reply != QMessageBox::Yes) return;

    QSqlDatabase db = QSqlDatabase::database();
    db.transaction();

    // 1. Demote employees using this role to 'user'
    QSqlQuery q1;
    q1.prepare("UPDATE employees SET role = 'user' WHERE role = ?");
    q1.addBindValue(roleName);
    if (!q1.exec()) {
        db.rollback();
        QMessageBox::critical(this, "错误", "降级员工角色失败！\n" + q1.lastError().text());
        return;
    }

    // 2. Delete role (foreign keys cascade delete role_permissions mapping)
    QSqlQuery q2;
    q2.prepare("DELETE FROM roles WHERE role_name = ?");
    q2.addBindValue(roleName);
    if (!q2.exec()) {
        db.rollback();
        QMessageBox::critical(this, "错误", "删除角色失败！\n" + q2.lastError().text());
        return;
    }

    db.commit();

    logAction("删除角色", roleName);
    loadRoles();

    emit GlobalEvents::instance()->dataChanged();
    QMessageBox::information(this, "成功", QString("已成功删除角色 '%1'！").arg(roleName));
}

void RbacTab::savePermissions()
{
    QListWidgetItem *current = m_roleList->currentItem();
    if (!current) return;

    QString roleName = current->text();
    int roleId = getRoleId(roleName);
    if (roleId == -1) {
        QMessageBox::critical(this, "错误", "未找到对应的角色ID！");
        return;
    }

    QSqlDatabase db = QSqlDatabase::database();
    db.transaction();

    // 1. Clear existing role permission mappings
    QSqlQuery q1;
    q1.prepare("DELETE FROM role_permissions WHERE role_id = ?");
    q1.addBindValue(roleId);
    if (!q1.exec()) {
        db.rollback();
        QMessageBox::critical(this, "错误", "清除旧权限映射失败！\n" + q1.lastError().text());
        return;
    }

    // 2. Insert new checked mappings
    for (auto it = m_permCheckBoxes.begin(); it != m_permCheckBoxes.end(); ++it) {
        if (it.value()->isChecked()) {
            QSqlQuery q2;
            q2.prepare("INSERT INTO role_permissions (role_id, permission_id) "
                       "SELECT ?, permission_id FROM permissions WHERE permission_key = ?");
            q2.addBindValue(roleId);
            q2.addBindValue(it.key());
            if (!q2.exec()) {
                db.rollback();
                QMessageBox::critical(this, "错误", QString("无法插入权限 '%1'！\n%2").arg(it.key(), q2.lastError().text()));
                return;
            }
        }
    }

    db.commit();

    // 3. If currently logged-in user shares this role, refresh session cached permissions immediately!
    if (SessionManager::instance()->role == roleName) {
        SessionManager::instance()->reloadPermissions();
    }

    logAction("修改角色权限", roleName);
    QMessageBox::information(this, "成功", QString("角色 '%1' 的权限配置已成功保存！").arg(roleName));
}

int RbacTab::getRoleId(const QString &roleName)
{
    QSqlQuery q;
    q.prepare("SELECT role_id FROM roles WHERE role_name = ?");
    q.addBindValue(roleName);
    if (q.exec() && q.next()) {
        return q.value(0).toInt();
    }
    return -1;
}

void RbacTab::logAction(const QString &action, const QString &target)
{
    if (m_log) {
        m_log(action, target);
    } else {
        QSqlQuery q;
        q.prepare("INSERT INTO audit_logs(emp_id,emp_name,action,target,log_time) VALUES(?,?,?,?,NOW())");
        q.addBindValue(SessionManager::instance()->empId);
        q.addBindValue(SessionManager::instance()->empName);
        q.addBindValue(action);
        q.addBindValue(target);
        q.exec();
        emit GlobalEvents::instance()->auditRefresh();
    }
}
