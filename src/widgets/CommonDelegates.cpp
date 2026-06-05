#include "CommonDelegates.h"
#include "../core/SessionManager.h"
#include <QPainter>
#include <QEvent>
#include <QMouseEvent>
#include <QSqlQuery>
#include <QSqlError>
#include <QSqlTableModel>
#include <QPixmap>
#include <QMap>

static QString fieldDisplayName(const QString &field)
{
    static QMap<QString, QString> map = {
        {"phone", "联系电话"}, {"education", "学历"},
        {"marital_status", "婚姻状况"}, {"position", "岗位"}, {"gender", "性别"}
    };
    return map.value(field, field);
}

QIcon createAvatarIcon(const QString &name)
{
    QPixmap pix(24, 24);
    pix.fill(Qt::transparent);
    QPainter painter(&pix);
    painter.setRenderHint(QPainter::Antialiasing);
    uint hash = qHash(name);
    int hue = hash % 360;
    QColor bgColor = QColor::fromHsl(hue, 120, 160);
    painter.setBrush(bgColor);
    painter.setPen(Qt::NoPen);
    painter.drawEllipse(0, 0, 24, 24);
    painter.setPen(Qt::white);
    QFont f = painter.font();
    f.setBold(true);
    f.setPointSize(9);
    painter.setFont(f);
    painter.drawText(pix.rect(), Qt::AlignCenter, name.left(1));
    return QIcon(pix);
}

void AttendanceStatusDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QStyleOptionViewItem opt = option;
    initStyleOption(&opt, index);

    QString status = index.data(Qt::DisplayRole).toString();

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);

    if (opt.state & QStyle::State_Selected) {
        painter->fillRect(opt.rect, QColor(0xf1, 0xf5, 0xf9));
        opt.state &= ~QStyle::State_Selected;
    }

    QColor bgColor, textColor;
    if (status == "正常") {
        bgColor = QColor(0xdc, 0xfc, 0xe7); // Light Green
        textColor = QColor(0x15, 0x80, 0x3d);
    } else if (status == "迟到") {
        bgColor = QColor(0xfe, 0xf3, 0xc7); // Light Yellow
        textColor = QColor(0xb4, 0x53, 0x09);
    } else if (status == "早退") {
        bgColor = QColor(0xfe, 0xe2, 0xe2); // Light Red
        textColor = QColor(0xb9, 0x1c, 0x1c);
    } else if (status == "迟到/早退") {
        bgColor = QColor(0xff, 0xed, 0xd5); // Light Orange
        textColor = QColor(0xc2, 0x41, 0x0c);
    } else if (status == "缺卡") {
        bgColor = QColor(0xf1, 0xf5, 0xf9); // Light Gray
        textColor = QColor(0x47, 0x55, 0x69);
    } else if (status == "请假") {
        bgColor = QColor(0xdb, 0xea, 0xfe); // Light Blue
        textColor = QColor(0x1d, 0x4e, 0xd8);
    } else {
        bgColor = QColor(0xf1, 0xf5, 0xf9);
        textColor = QColor(0x47, 0x55, 0x69);
    }

    int paddingX = 8;
    int paddingY = 3;
    QFont font = opt.font;
    font.setPointSize(9);
    font.setBold(true);
    painter->setFont(font);

    QFontMetrics fm(font);
    int textWidth = fm.horizontalAdvance(status);
    int textHeight = fm.height();

    int pillWidth = textWidth + paddingX * 2;
    int pillHeight = textHeight + paddingY * 2;

    int pillX = opt.rect.x() + (opt.rect.width() - pillWidth) / 2;
    int pillY = opt.rect.y() + (opt.rect.height() - pillHeight) / 2;

    QRect pillRect(pillX, pillY, pillWidth, pillHeight);
    painter->setBrush(bgColor);
    painter->setPen(Qt::NoPen);
    painter->drawRoundedRect(pillRect, 4, 4);

    painter->setPen(textColor);
    painter->drawText(pillRect, Qt::AlignCenter, status);

    painter->restore();
}

void RequestStatusDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QStyleOptionViewItem opt = option;
    initStyleOption(&opt, index);

    QString status = index.data(Qt::DisplayRole).toString();

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);

    if (opt.state & QStyle::State_Selected) {
        painter->fillRect(opt.rect, QColor(0xf1, 0xf5, 0xf9));
        opt.state &= ~QStyle::State_Selected;
    }

    QColor bgColor, textColor;
    if (status == "待审批" || status == "审批中") {
        bgColor = QColor(0xfe, 0xf3, 0xc7); // Light Yellow
        textColor = QColor(0xb4, 0x53, 0x09);
        status = "待审批";
    } else if (status == "已同意" || status == "已通过" || status == "已同意补卡") {
        bgColor = QColor(0xdc, 0xfc, 0xe7); // Light Green
        textColor = QColor(0x15, 0x80, 0x3d);
        status = "已同意";
    } else if (status == "已拒绝" || status == "已驳回") {
        bgColor = QColor(0xfe, 0xe2, 0xe2); // Light Red
        textColor = QColor(0xb9, 0x1c, 0x1c);
        status = "已拒绝";
    } else {
        bgColor = QColor(0xf1, 0xf5, 0xf9);
        textColor = QColor(0x47, 0x55, 0x69);
    }

    int paddingX = 8;
    int paddingY = 3;
    QFont font = opt.font;
    font.setPointSize(9);
    font.setBold(true);
    painter->setFont(font);

    QFontMetrics fm(font);
    int textWidth = fm.horizontalAdvance(status);
    int textHeight = fm.height();

    int pillWidth = textWidth + paddingX * 2;
    int pillHeight = textHeight + paddingY * 2;

    int pillX = opt.rect.x() + (opt.rect.width() - pillWidth) / 2;
    int pillY = opt.rect.y() + (opt.rect.height() - pillHeight) / 2;

    QRect pillRect(pillX, pillY, pillWidth, pillHeight);
    painter->setBrush(bgColor);
    painter->setPen(Qt::NoPen);
    painter->drawRoundedRect(pillRect, 4, 4);

    painter->setPen(textColor);
    painter->drawText(pillRect, Qt::AlignCenter, status);

    painter->restore();
}

void MakeupTypeDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QStyleOptionViewItem opt = option;
    initStyleOption(&opt, index);

    QString type = index.data(Qt::DisplayRole).toString();
    if (type == "clock_in") {
        opt.text = "上班签到";
    } else if (type == "clock_out") {
        opt.text = "下班签退";
    }

    QStyledItemDelegate::paint(painter, opt, index);
}

void EmployeeAvatarDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QStyleOptionViewItem opt = option;
    initStyleOption(&opt, index);

    QString name = index.data(Qt::DisplayRole).toString();
    if (name.isEmpty()) {
        QStyledItemDelegate::paint(painter, opt, index);
        return;
    }

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);

    // Highlight selection
    if (opt.state & QStyle::State_Selected) {
        painter->fillRect(opt.rect, QColor(0xf1, 0xf5, 0xf9));
        opt.state &= ~QStyle::State_Selected;
    }

    // Draw circle avatar
    uint hash = qHash(name);
    int hue = hash % 360;
    QColor avatarBg = QColor::fromHsl(hue, 120, 165);

    int avatarSize = 24;
    int paddingX = 8;
    int avatarX = opt.rect.x() + paddingX;
    int avatarY = opt.rect.y() + (opt.rect.height() - avatarSize) / 2;
    QRect avatarRect(avatarX, avatarY, avatarSize, avatarSize);

    painter->setBrush(avatarBg);
    painter->setPen(Qt::NoPen);
    painter->drawEllipse(avatarRect);

    // Draw letter
    painter->setPen(Qt::white);
    QFont f = opt.font;
    f.setBold(true);
    f.setPointSize(9);
    painter->setFont(f);
    painter->drawText(avatarRect, Qt::AlignCenter, name.left(1));

    // Draw name label
    QRect textRect = opt.rect;
    textRect.setLeft(avatarX + avatarSize + 8);
    painter->setPen(QColor(0x0f, 0x17, 0x2a));
    f.setBold(false);
    f.setPointSize(10);
    painter->setFont(f);
    painter->drawText(textRect, Qt::AlignVCenter | Qt::AlignLeft, name);

    painter->restore();
}

void PerformanceStatusDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QStyleOptionViewItem opt = option;
    initStyleOption(&opt, index);

    QString status = index.data(Qt::DisplayRole).toString();

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);

    if (opt.state & QStyle::State_Selected) {
        painter->fillRect(opt.rect, QColor(0xf1, 0xf5, 0xf9));
        opt.state &= ~QStyle::State_Selected;
    }

    bool isPublished = (status == "已发布");

    int trackWidth = 56;
    int trackHeight = 22;
    int trackX = opt.rect.x() + (opt.rect.width() - trackWidth) / 2;
    int trackY = opt.rect.y() + (opt.rect.height() - trackHeight) / 2;
    QRect trackRect(trackX, trackY, trackWidth, trackHeight);

    QColor trackColor = isPublished ? QColor(0x22, 0xc5, 0x5e) : QColor(0x94, 0xa3, 0xb8); // Green vs Slate-Gray
    QColor knobColor = Qt::white;

    // Draw switch track
    painter->setBrush(trackColor);
    painter->setPen(Qt::NoPen);
    painter->drawRoundedRect(trackRect, trackHeight / 2, trackHeight / 2);

    // Draw switch knob
    int knobSize = 16;
    int knobY = trackY + (trackHeight - knobSize) / 2;
    int knobX = isPublished ? (trackX + trackWidth - knobSize - 3) : (trackX + 3);
    QRect knobRect(knobX, knobY, knobSize, knobSize);
    painter->setBrush(knobColor);
    painter->drawEllipse(knobRect);

    // Draw inline text
    QFont font = opt.font;
    font.setPointSize(8);
    font.setBold(true);
    painter->setFont(font);
    painter->setPen(isPublished ? Qt::white : QColor(0xf1, 0xf5, 0xf9));

    QString text = isPublished ? "发布" : "草稿";
    if (isPublished) {
        QRect textRect(trackX + 4, trackY, trackWidth - knobSize - 6, trackHeight);
        painter->drawText(textRect, Qt::AlignCenter, text);
    } else {
        QRect textRect(trackX + knobSize + 2, trackY, trackWidth - knobSize - 6, trackHeight);
        painter->drawText(textRect, Qt::AlignCenter, text);
    }

    painter->restore();
}

bool PerformanceStatusDelegate::editorEvent(QEvent *event, QAbstractItemModel *model, const QStyleOptionViewItem &option, const QModelIndex &index)
{
    if (event->type() == QEvent::MouseButtonRelease) {
        QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
        if (mouseEvent->button() == Qt::LeftButton) {
            if (!SessionManager::instance()->hasPermission("evaluate_performance")) {
                return false;
            }
            QString currentStatus = index.data(Qt::DisplayRole).toString();
            QString newStatus = (currentStatus == "已发布") ? "未发布" : "已发布";

            int row = index.row();
            int scoreId = model->index(row, 0).data().toInt();

            bool ok = false;
            {
                QSqlQuery q;
                q.prepare("UPDATE performance_scores SET status=? WHERE score_id=?");
                q.addBindValue(newStatus);
                q.addBindValue(scoreId);
                ok = q.exec();
                q.finish();
            }
            if (ok) {
                model->setData(index, newStatus, Qt::EditRole);
                QSqlTableModel *sqlModel = qobject_cast<QSqlTableModel*>(model);
                if (sqlModel) sqlModel->select();
                return true;
            }
        }
    }
    return QStyledItemDelegate::editorEvent(event, model, option, index);
}

void ProfileChangeDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QStyleOptionViewItem opt = option;
    initStyleOption(&opt, index);

    if (index.column() == 2) { // 字段 column
        QString rawField = index.data(Qt::DisplayRole).toString();
        opt.text = fieldDisplayName(rawField);
        QStyledItemDelegate::paint(painter, opt, index);
    }
    else if (index.column() == 5) { // 状态 column
        QString status = index.data(Qt::DisplayRole).toString();
        
        painter->save();
        painter->setRenderHint(QPainter::Antialiasing);

        // Draw background pill/badge
        QColor bgColor, textColor;
        if (status == "待审批" || status == "审批中") {
            bgColor = QColor(0xea, 0xb3, 0x08); // Yellow
            textColor = QColor(0xff, 0xff, 0xff);
            status = "审批中";
        } else if (status == "已同意" || status == "已通过") {
            bgColor = QColor(0x22, 0xc5, 0x5e); // Green
            textColor = QColor(0xff, 0xff, 0xff);
            status = "已通过";
        } else if (status == "已拒绝" || status == "已驳回") {
            bgColor = QColor(0xef, 0x44, 0x44); // Red
            textColor = QColor(0xff, 0xff, 0xff);
            status = "已驳回";
        } else {
            bgColor = QColor(0x94, 0xa3, 0xb8); // Gray
            textColor = QColor(0xff, 0xff, 0xff);
        }

        // Draw pill rectangle
        int paddingX = 8;
        int paddingY = 3;
        QFont font = opt.font;
        font.setPointSize(9);
        font.setBold(true);
        painter->setFont(font);

        QFontMetrics fm(font);
        int textWidth = fm.horizontalAdvance(status);
        int textHeight = fm.height();

        int pillWidth = textWidth + paddingX * 2;
        int pillHeight = textHeight + paddingY * 2;

        // Center the pill in the cell
        int pillX = opt.rect.x() + (opt.rect.width() - pillWidth) / 2;
        int pillY = opt.rect.y() + (opt.rect.height() - pillHeight) / 2;

        QRect pillRect(pillX, pillY, pillWidth, pillHeight);
        painter->setBrush(bgColor);
        painter->setPen(Qt::NoPen);
        painter->drawRoundedRect(pillRect, pillHeight / 2.0, pillHeight / 2.0);

        // Draw text
        painter->setPen(textColor);
        painter->drawText(pillRect, Qt::AlignCenter, status);

        painter->restore();
    }
    else {
        if (opt.state & QStyle::State_Selected) {
            painter->save();
            painter->fillRect(opt.rect, QColor(0xf1, 0xf5, 0xf9));
            painter->restore();
            opt.state &= ~QStyle::State_Selected;
            opt.palette.setColor(QPalette::Text, QColor(0x1e, 0x29, 0x3b));
            opt.palette.setColor(QPalette::HighlightedText, QColor(0x1e, 0x29, 0x3b));
        }
        QStyledItemDelegate::paint(painter, opt, index);
    }
}
