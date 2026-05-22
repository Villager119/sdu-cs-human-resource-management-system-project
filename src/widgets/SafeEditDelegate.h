#ifndef SAFEEDITDELEGATE_H
#define SAFEEDITDELEGATE_H

#include <QStyledItemDelegate>
#include <QLineEdit>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QDateTimeEdit>

class SafeEditDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    explicit SafeEditDelegate(QObject *parent = nullptr) : QStyledItemDelegate(parent) {}

    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const override
    {
        QWidget *editor = QStyledItemDelegate::createEditor(parent, option, index);
        if (editor) {
            // Apply strict stylesheet rules to ensure text visibility regardless of system theme
            editor->setStyleSheet(
                "color: #0f172a !important;"
                "background-color: #ffffff !important;"
                "selection-color: #ffffff !important;"
                "selection-background-color: #2563eb !important;"
                "border: 1px solid #3b82f6 !important;"
                "border-radius: 4px !important;"
            );
        }
        return editor;
    }
};

#endif // SAFEEDITDELEGATE_H
