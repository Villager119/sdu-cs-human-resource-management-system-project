#include "ServerSettingsDialog.h"
#include "../utils/DbUtils.h"
#include <QFormLayout>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QDialogButtonBox>
#include <QSettings>
#include <QFileInfo>
#include <QSqlDatabase>
#include <QSqlError>
#include <QMessageBox>
#include <QTcpSocket>
#include <QNetworkInterface>
#include <QApplication>

ServerSettingsDialog::ServerSettingsDialog(const QString &configPath, QWidget *parent)
    : QDialog(parent)
    , m_configPath(configPath)
{
    setWindowTitle("服务器设置");
    setMinimumWidth(420);

    auto *mainLayout = new QVBoxLayout(this);

    // 表单
    auto *form = new QFormLayout;
    m_driverEdit = new QLineEdit(this);
    m_serverEdit = new QLineEdit(this);
    m_portSpin   = new QSpinBox(this);
    m_portSpin->setRange(1, 65535);
    m_portSpin->setValue(3306);
    m_dbEdit     = new QLineEdit(this);
    m_uidEdit    = new QLineEdit(this);
    m_pwdEdit    = new QLineEdit(this);
    m_pwdEdit->setEchoMode(QLineEdit::Password);

    form->addRow("驱动:", m_driverEdit);
    form->addRow("服务器:", m_serverEdit);
    form->addRow("端口:", m_portSpin);
    form->addRow("数据库:", m_dbEdit);
    form->addRow("用户名:", m_uidEdit);
    form->addRow("密码:", m_pwdEdit);
    mainLayout->addLayout(form);

    // 扫描区域
    auto *scanLayout = new QHBoxLayout;
    m_scanBtn = new QPushButton("扫描局域网", this);
    m_progressBar = new QProgressBar(this);
    m_progressBar->setVisible(false);
    m_progressBar->setMaximum(0); // 不确定进度
    scanLayout->addWidget(m_scanBtn);
    scanLayout->addWidget(m_progressBar);
    mainLayout->addLayout(scanLayout);

    m_scanList = new QListWidget(this);
    m_scanList->setMaximumHeight(100);
    mainLayout->addWidget(m_scanList);

    // 按钮区
    auto *btnLayout = new QHBoxLayout;
    m_testBtn = new QPushButton("测试连接", this);
    auto *saveBtn = new QPushButton("保存", this);
    auto *cancelBtn = new QPushButton("取消", this);
    btnLayout->addWidget(m_testBtn);
    btnLayout->addStretch();
    btnLayout->addWidget(saveBtn);
    btnLayout->addWidget(cancelBtn);
    mainLayout->addLayout(btnLayout);

    // 读取当前配置
    QSettings settings(configPath, QSettings::IniFormat);
    m_driverEdit->setText(settings.value("Database/Driver", "MySQL ODBC 9.2 Unicode Driver").toString());
    m_serverEdit->setText(settings.value("Database/Server", "127.0.0.1").toString());
    m_portSpin->setValue(settings.value("Database/Port", 3306).toInt());
    m_dbEdit->setText(settings.value("Database/Database", "hrms_db").toString());
    m_uidEdit->setText(settings.value("Database/UID", "root").toString());
    m_pwdEdit->setText(settings.value("Database/PWD", "").toString());

    // 连接信号
    connect(m_scanBtn, &QPushButton::clicked, this, &ServerSettingsDialog::onScan);
    connect(m_scanList, &QListWidget::itemDoubleClicked, this, [this](QListWidgetItem *item) {
        m_serverEdit->setText(item->text());
    });
    connect(m_testBtn, &QPushButton::clicked, this, &ServerSettingsDialog::onTest);
    connect(saveBtn, &QPushButton::clicked, this, &ServerSettingsDialog::onSave);
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
}

QStringList ServerSettingsDialog::scanSubnet()
{
    QStringList found;
    QStringList localIps;

    const auto interfaces = QNetworkInterface::allInterfaces();
    QStringList subnets;
    for (const auto &iface : interfaces) {
        if (iface.flags() & QNetworkInterface::IsLoopBack)
            continue;

        // 跳过虚拟网卡
        QString name = iface.humanReadableName().toLower();
        if (name.contains("vmware") || name.contains("virtualbox")
            || name.contains("vbox") || name.contains("vethernet")
            || name.contains("hyper-v"))
            continue;

        for (const auto &entry : iface.addressEntries()) {
            QHostAddress ip = entry.ip();
            if (ip.protocol() != QAbstractSocket::IPv4Protocol || ip.isLoopback())
                continue;
            QHostAddress mask = entry.netmask();
            if (mask.isNull())
                continue;
            quint32 ip4 = ip.toIPv4Address();
            quint32 mk4 = mask.toIPv4Address();
            if (mk4 == 0)
                continue;

            localIps.append(ip.toString());

            // 跳过常见的虚拟网卡网段
            quint32 net = ip4 & mk4;
            QString prefix = QString("%1.%2.%3.")
                .arg((net >> 24) & 0xFF)
                .arg((net >> 16) & 0xFF)
                .arg((net >> 8)  & 0xFF);
            if (prefix.startsWith("192.168.56.")   // VMware host-only
                || prefix.startsWith("192.168.73.")  // VirtualBox host-only
                || prefix.startsWith("192.168.153.")) // 其它虚拟网卡
                continue;

            subnets.append(prefix);
        }
    }

    subnets.removeDuplicates();
    if (subnets.isEmpty())
        return found;

    for (const auto &prefix : subnets) {
        for (int host = 1; host < 128; ++host) {
            QApplication::processEvents();
            QString ip = prefix + QString::number(host);
            if (localIps.contains(ip))       // 跳过本机
                continue;
            if (ip == m_serverEdit->text())  // 跳过已在配置中的
                continue;
            QTcpSocket sock;
            sock.connectToHost(ip, m_portSpin->value());
            if (sock.waitForConnected(80)) {
                found.append(ip);
                sock.disconnectFromHost();
            }
        }
    }
    return found;
}

void ServerSettingsDialog::onScan()
{
    m_scanList->clear();
    m_scanBtn->setEnabled(false);
    m_progressBar->setVisible(true);
    m_testBtn->setEnabled(false);
    QApplication::processEvents();

    QStringList results = scanSubnet();

    m_progressBar->setVisible(false);
    m_scanBtn->setEnabled(true);
    m_testBtn->setEnabled(true);

    if (results.isEmpty()) {
        m_scanList->addItem("未找到MySQL服务器");
    } else {
        for (const auto &ip : results)
            m_scanList->addItem(ip);
    }
}

void ServerSettingsDialog::onTest()
{
    QString driver = m_driverEdit->text().trimmed();
    QString server = m_serverEdit->text().trimmed();
    int port = m_portSpin->value();
    QString database = m_dbEdit->text().trimmed();
    QString uid = m_uidEdit->text().trimmed();
    QString pwd = m_pwdEdit->text();

    {
        QSqlDatabase testDb = QSqlDatabase::addDatabase("QODBC", "_test_conn_");
        testDb.setDatabaseName(buildDsn(driver, server, port, database, uid, pwd));

        if (testDb.open()) {
            QMessageBox::information(this, "连接成功", "数据库连接测试成功！");
            testDb.close();
        } else {
            QMessageBox::warning(this, "连接失败", testDb.lastError().text());
        }
    }
    QSqlDatabase::removeDatabase("_test_conn_");
}

void ServerSettingsDialog::onSave()
{
    QSettings settings(m_configPath, QSettings::IniFormat);
    settings.setValue("Database/Driver",   m_driverEdit->text().trimmed());
    settings.setValue("Database/Server",   m_serverEdit->text().trimmed());
    settings.setValue("Database/Port",     m_portSpin->value());
    settings.setValue("Database/Database", m_dbEdit->text().trimmed());
    settings.setValue("Database/UID",      m_uidEdit->text().trimmed());
    settings.setValue("Database/PWD",      QString(m_pwdEdit->text().toUtf8().toBase64()));
    settings.sync();

    accept();
}
