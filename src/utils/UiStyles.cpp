#include "UiStyles.h"

namespace UiStyles {

QString tabWidget()
{
    return "QTabWidget::panel { border: none; background-color: #ffffff; }"
           "QTabBar::tab { font-size: 13px; padding: 6px 14px; }";
}

QString panelFrame(const QString &objectName)
{
    return QString("QFrame#%1 { background-color: #ffffff; border: 1px solid #e2e8f0; border-radius: 8px; }")
        .arg(objectName);
}

QString sectionTitle(bool primary)
{
    return QString("font-size: 15px; font-weight: bold; color: %1; border: none; background: transparent;")
        .arg(primary ? "#1d4ed8" : "#1e293b");
}

QString strongText()
{
    return "font-weight: bold; color: #1e293b;";
}

QString textEdit()
{
    return "QTextEdit {"
           "  border: 1px solid #cbd5e1;"
           "  border-radius: 6px;"
           "  padding: 8px;"
           "  font-size: 13px;"
           "}"
           "QTextEdit:focus { border: 1px solid #3b82f6; }";
}

QString comboBox()
{
    return "QComboBox {"
           "  border: 1px solid #cbd5e1;"
           "  border-radius: 6px;"
           "  padding: 6px 10px;"
           "  font-size: 13px;"
           "  background: #ffffff;"
           "}";
}

QString primaryButton()
{
    return "QPushButton {"
           "  background-color: #2563eb;"
           "  color: white;"
           "  border: none;"
           "  border-radius: 6px;"
           "  padding: 8px 20px;"
           "  font-weight: bold;"
           "}"
           "QPushButton:hover { background-color: #1d4ed8; }";
}

QString successButton()
{
    return "QPushButton { background-color: #22c55e; color: white; border: none; border-radius: 6px; padding: 6px 16px; font-weight: bold; } "
           "QPushButton:hover { background-color: #16a34a; }";
}

QString dangerButton()
{
    return "QPushButton { background-color: #ef4444; color: white; border: none; border-radius: 6px; padding: 6px 16px; font-weight: bold; } "
           "QPushButton:hover { background-color: #dc2626; }";
}

QString secondaryButton()
{
    return "QPushButton { background-color: #f1f5f9; color: #475569; border: 1px solid #cbd5e1; border-radius: 6px; padding: 6px 16px; } "
           "QPushButton:hover { background-color: #e2e8f0; }";
}

QString actionButton()
{
    return primaryButton() +
           "QPushButton:disabled { background-color: #f1f5f9; color: #94a3b8; border: 1px solid #e2e8f0; }";
}

QString tableHeader()
{
    return "QHeaderView::section {"
           "  background-color: #f1f5f9;"
           "  color: #475569;"
           "  padding: 8px;"
           "  border: none;"
           "  font-weight: bold;"
           "  border-bottom: 2px solid #e2e8f0;"
           "}";
}

QString tableView()
{
    return "QTableView {"
           "  border: none;"
           "  background-color: #ffffff;"
           "  gridline-color: #f1f5f9;"
           "  selection-background-color: #f1f5f9;"
           "  selection-color: #0f172a;"
           "}"
           "QTableView::item {"
           "  padding: 8px;"
           "  border-bottom: 1px solid #f1f5f9;"
           "}";
}

}

