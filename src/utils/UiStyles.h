#ifndef UISTYLES_H
#define UISTYLES_H

#include <QString>

namespace UiStyles {

QString tabWidget();
QString panelFrame(const QString &objectName);
QString sectionTitle(bool primary = false);
QString strongText();
QString textEdit();
QString comboBox();
QString primaryButton();
QString successButton();
QString dangerButton();
QString secondaryButton();
QString actionButton();
QString tableHeader();
QString tableView();

}

#endif
