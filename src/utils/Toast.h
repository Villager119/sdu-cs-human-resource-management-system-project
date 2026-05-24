#ifndef TOAST_H
#define TOAST_H

#include <QFrame>
#include <QLabel>
#include <QHBoxLayout>
#include <QTimer>
#include <QGraphicsOpacityEffect>
#include <QPropertyAnimation>
#include <QGraphicsDropShadowEffect>

class Toast : public QFrame {
public:
    enum Type { Success, Warning, Error, Info };

    static void show(QWidget *parent, const QString &message, Type type = Info) {
        if (!parent) return;
        QWidget *topLevel = parent->window();
        Toast *toast = new Toast(topLevel, message, type);
        toast->QWidget::show();
    }

private:
    Toast(QWidget *parent, const QString &message, Type type) : QFrame(parent) {
        setAttribute(Qt::WA_DeleteOnClose);
        
        // Frameless overlay, transparent for mouse to avoid stealing clicks
        setWindowFlags(Qt::FramelessWindowHint | Qt::SubWindow);
        setAttribute(Qt::WA_TransparentForMouseEvents);
        
        QHBoxLayout *layout = new QHBoxLayout(this);
        layout->setContentsMargins(14, 10, 14, 10);
        layout->setSpacing(8);
        
        QLabel *iconLabel = new QLabel(this);
        iconLabel->setStyleSheet("border: none; background: transparent; font-size: 14px;");
        
        QString bgStyle;
        if (type == Success) {
            iconLabel->setText("✅");
            bgStyle = "background-color: #059669; border: 1px solid #047857;"; // Emerald
        } else if (type == Warning) {
            iconLabel->setText("⚠️");
            bgStyle = "background-color: #d97706; border: 1px solid #b45309;"; // Amber
        } else if (type == Error) {
            iconLabel->setText("❌");
            bgStyle = "background-color: #dc2626; border: 1px solid #b91c1c;"; // Red
        } else {
            iconLabel->setText("ℹ️");
            bgStyle = "background-color: #2563eb; border: 1px solid #1d4ed8;"; // Blue
        }
        
        setStyleSheet(QString("Toast { %1 color: #ffffff; border-radius: 8px; }").arg(bgStyle));
        
        QLabel *textLabel = new QLabel(message, this);
        textLabel->setStyleSheet("color: #ffffff; border: none; background: transparent; font-size: 13px; font-weight: 500;");
        textLabel->setWordWrap(true);
        
        layout->addWidget(iconLabel);
        layout->addWidget(textLabel);
        
        // Add shadow for float/elevation visual style
        QGraphicsDropShadowEffect *shadow = new QGraphicsDropShadowEffect(this);
        shadow->setBlurRadius(12);
        shadow->setOffset(0, 3);
        shadow->setColor(QColor(0, 0, 0, 50));
        setGraphicsEffect(shadow);

        adjustSize();
        
        // Position at the bottom-right of parent
        int margin = 20;
        int x = parent->width() - width() - margin;
        // Lift slightly to clear statusbars
        int y = parent->height() - height() - margin - 22;
        move(x, y);
        
        // Fade-in animation
        QGraphicsOpacityEffect *opacityEffect = new QGraphicsOpacityEffect(this);
        // Note: setting multiple graphics effects isn't directly supported by setGraphicsEffect, 
        // but since we want both shadow and opacity, we can apply opacity to child controls, or apply opacity on parent.
        // Actually, we can use paintEvent for opacity, or just animate windowOpacity on Windows.
        // In Qt, setGraphicsEffect replaces the previous one, so we can't have both on the same widget.
        // Let's use opacityEffect for the fade, as it's cleaner, and drop the shadow if they conflict, 
        // or apply shadow to textLabel and iconLabel, but shadow on the whole frame is nicer.
        // Let's just use opacityEffect on the frame. It's more critical for fading out.
        setGraphicsEffect(opacityEffect);
        
        QPropertyAnimation *fadeIn = new QPropertyAnimation(opacityEffect, "opacity");
        fadeIn->setDuration(250);
        fadeIn->setStartValue(0.0);
        fadeIn->setEndValue(1.0);
        fadeIn->start(QAbstractAnimation::DeleteWhenStopped);
        
        QTimer::singleShot(2200, this, [this, opacityEffect]() {
            QPropertyAnimation *fadeOut = new QPropertyAnimation(opacityEffect, "opacity");
            fadeOut->setDuration(300);
            fadeOut->setStartValue(1.0);
            fadeOut->setEndValue(0.0);
            connect(fadeOut, &QPropertyAnimation::finished, this, &Toast::close);
            fadeOut->start(QAbstractAnimation::DeleteWhenStopped);
        });
    }
};

#endif // TOAST_H
