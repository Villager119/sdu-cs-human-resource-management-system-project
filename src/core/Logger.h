#ifndef LOGGER_H
#define LOGGER_H

#include <QString>
#include <QFile>
#include <QTextStream>
#include <QMutex>
#include <QDateTime>

class Logger
{
public:
    static Logger *instance();
    static void init(const QString &filePath);
    void log(const QString &level, const QString &message);
    QString filePath() const { return m_filePath; }

private:
    Logger() = default;
    QFile m_file;
    QMutex m_mutex;
    QString m_filePath;
};

// 便捷宏
#define LOG_INFO(msg)  Logger::instance()->log("INFO", msg)
#define LOG_WARN(msg)  Logger::instance()->log("WARN", msg)
#define LOG_ERROR(msg) Logger::instance()->log("ERROR",msg)

#endif
