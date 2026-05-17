#include "MCPStdioTransport.h"

#include <QJsonDocument>
#include <QMetaObject>
#include <QtGlobal>

#ifdef Q_OS_WIN
#include <io.h>
#include <fcntl.h>
#define STDIN_FILENO 0
#define READ_FN _read
#else
#include <unistd.h>
#define READ_FN ::read
#endif

// ---- StdinReaderThread ----

StdinReaderThread::StdinReaderThread(QObject *owner, QObject *parent)
    : QThread(parent), m_owner(owner), m_stopFlag(0) {}

void StdinReaderThread::stop()
{
    m_stopFlag.storeRelaxed(1);
}

void StdinReaderThread::run()
{
    QByteArray buffer;
    char rawBuf[4096];

    while (!m_stopFlag.loadRelaxed())
    {
        int bytesRead = READ_FN(STDIN_FILENO, rawBuf, sizeof(rawBuf));
        if (bytesRead <= 0)
            break;

        buffer.append(rawBuf, bytesRead);

        // Split by newline, emit complete lines as JSON
        int nlIdx;
        while ((nlIdx = buffer.indexOf('\n')) >= 0)
        {
            QByteArray line = buffer.left(nlIdx).trimmed();
            buffer.remove(0, nlIdx + 1);
            if (line.isEmpty()) continue;

            QJsonParseError err;
            QJsonDocument doc = QJsonDocument::fromJson(line, &err);
            if (err.error == QJsonParseError::NoError && doc.isObject())
            {
                QMetaObject::invokeMethod(m_owner, "messageReceived",
                    Qt::QueuedConnection, Q_ARG(QJsonObject, doc.object()));
            }
        }
    }
}

// ---- MCPStdioTransport ----

MCPStdioTransport::MCPStdioTransport(QObject *parent)
    : QObject(parent), m_readerThread(nullptr)
{
}

MCPStdioTransport::~MCPStdioTransport()
{
    stop();
}

void MCPStdioTransport::start()
{
#ifdef Q_OS_WIN
    // Set stdin/stdout to binary mode on Windows
    _setmode(STDIN_FILENO, _O_BINARY);
    _setmode(1, _O_BINARY);
#endif
    // Disable stdout buffering
    setvbuf(stdout, nullptr, _IONBF, 0);

    m_readerThread = new StdinReaderThread(this, this);
    connect(m_readerThread, &StdinReaderThread::messageReceived,
            this, &MCPStdioTransport::messageReceived);
    connect(m_readerThread, &StdinReaderThread::finished,
            this, &MCPStdioTransport::finished);

    m_readerThread->start();
}

void MCPStdioTransport::stop()
{
    if (m_readerThread)
    {
        m_readerThread->stop();
        m_readerThread->wait(3000);
        m_readerThread->deleteLater();
        m_readerThread = nullptr;
    }
}

void MCPStdioTransport::send(const QJsonObject &message)
{
    QMutexLocker locker(&m_writeMutex);
    QJsonDocument doc(message);
    QByteArray data = doc.toJson(QJsonDocument::Compact);
    std::fwrite(data.constData(), 1, data.size(), stdout);
    std::fputc('\n', stdout);
    std::fflush(stdout);
}
