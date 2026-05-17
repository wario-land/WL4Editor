#ifndef MCPTCPTRANSPORT_H
#define MCPTCPTRANSPORT_H

#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QJsonObject>
#include <QMap>
#include <QSet>

/// TCP+SSE transport for MCP.
/// Clients connect via HTTP: GET /sse for SSE stream, POST /message?sessionId=X for JSON-RPC.
class MCPTcpTransport : public QObject
{
    Q_OBJECT
public:
    explicit MCPTcpTransport(QObject *parent = nullptr);
    ~MCPTcpTransport() override;

    bool start(quint16 port = 9876);
    void stop();
    quint16 port() const { return m_port; }

    /// Send a JSON-RPC message to a specific SSE session.
    void sendToSession(const QString &sessionId, const QJsonObject &message);

    /// Broadcast to all SSE sessions.
    void broadcast(const QJsonObject &message);

signals:
    /// Emitted when a JSON-RPC request is received from any client.
    void messageReceived(const QJsonObject &message, const QString &sessionId);

private slots:
    void onNewConnection();
    void onReadyRead();
    void onDisconnected();

private:
    void handleHttpRequest(QTcpSocket *socket, const QByteArray &data);
    void sendHttpResponse(QTcpSocket *socket, int statusCode,
                          const QString &contentType, const QByteArray &body);
    void startSseStream(QTcpSocket *socket, const QString &sessionId);
    QString generateSessionId();

    QTcpServer *m_server;
    quint16 m_port;

    /// SSE sessions: sessionId -> QTcpSocket
    QMap<QString, QTcpSocket *> m_sseSessions;
    /// Reverse: QTcpSocket -> sessionId
    QMap<QTcpSocket *, QString> m_socketSessions;
    /// Sockets that haven't completed an HTTP request yet
    QMap<QTcpSocket *, QByteArray> m_pendingBuffers;
    /// Active SSE session IDs
    QSet<QString> m_activeSessions;
};

#endif // MCPTCPTRANSPORT_H
