#ifndef MCPSTDIOTRANSPORT_H
#define MCPSTDIOTRANSPORT_H

#include <QObject>
#include <QThread>
#include <QMutex>
#include <QJsonObject>

/// Worker thread that blocks on stdin, emits received JSON-RPC messages.
class StdinReaderThread : public QThread
{
    Q_OBJECT
public:
    explicit StdinReaderThread(QObject *owner, QObject *parent = nullptr);
    void stop();

signals:
    void messageReceived(const QJsonObject &message);

protected:
    void run() override;

private:
    QObject *m_owner;
    QAtomicInt m_stopFlag;
};

/// Writes JSON-RPC responses to stdout (thread-safe).
class MCPStdioTransport : public QObject
{
    Q_OBJECT
public:
    explicit MCPStdioTransport(QObject *parent = nullptr);
    ~MCPStdioTransport() override;

    void start();
    void stop();
    void send(const QJsonObject &message);

signals:
    void messageReceived(const QJsonObject &message);
    void finished();

private:
    StdinReaderThread *m_readerThread;
    QMutex m_writeMutex;
};

#endif // MCPSTDIOTRANSPORT_H
