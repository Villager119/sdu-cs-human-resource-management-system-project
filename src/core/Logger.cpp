#include "Logger.h"
#include <QDir>
#include <QDebug>

static Logger *s_instance = nullptr;

Logger *Logger::instance()
{
    if (!s_instance) {
        s_instance = new Logger;
    }
    return s_instance;
}

void Logger::init(const QString &filePath)
{
    auto *inst = instance();
    inst->m_filePath = filePath;
    inst->m_file.setFileName(filePath);
    if (!inst->m_file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text))
        qWarning() << "Logger: 无法打开日志文件" << filePath;
}

void Logger::log(const QString &level, const QString &message)
{
    QMutexLocker lock(&m_mutex);
    if (!m_file.isOpen()) return;
    QTextStream out(&m_file);
    out << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss")
        << " [" << level << "] " << message << "\n";
    out.flush();
}
