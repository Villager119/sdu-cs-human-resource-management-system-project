#include "ComboDelegate.h"
#include <QComboBox>

ComboDelegate::ComboDelegate(const QStringList &options, QObject *parent)
    : QStyledItemDelegate(parent), m_options(options) {}

ComboDelegate::ComboDelegate(QAbstractItemModel *model, int valueColumn, QObject *parent)
    : QStyledItemDelegate(parent), m_lookupModel(model), m_lookupColumn(valueColumn) {}

QWidget *ComboDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &, const QModelIndex &) const
{
    auto *cb = new QComboBox(parent);
    if (m_lookupModel) {
        cb->setModel(m_lookupModel);
        cb->setModelColumn(m_lookupColumn);
    } else {
        cb->addItems(m_options);
    }
    return cb;
}

void ComboDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    QString val = index.model()->data(index, Qt::EditRole).toString();
    auto *cb = static_cast<QComboBox *>(editor);
    if (m_lookupModel)
        cb->setCurrentText(val);
    else
        cb->setCurrentText(val);
}

void ComboDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
    auto *cb = static_cast<QComboBox *>(editor);
    model->setData(index, cb->currentText());
}
