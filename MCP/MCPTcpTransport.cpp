#include "MCPTcpTransport.h"

#include <QJsonDocument>
#include <QUuid>
#include <QUrl>
#include <QUrlQuery>
#include <QDebug>

MCPTcpTransport::MCPTcpTransport(QObject *parent)
    : QObject(parent), m_server(nullptr), m_port(0)
{
}

MCPTcpTransport::~MCPTcpTransport()
{
    stop();
}

bool MCPTcpTransport::start(quint16 port)
{
    if (m_server) return false;

    m_server = new QTcpServer(this);
    connect(m_server, &QTcpServer::newConnection, this, &MCPTcpTransport::onNewConnection);

    if (!m_server->listen(QHostAddress::LocalHost, port))
    {
        qWarning() << "MCP TCP: Failed to listen on port" << port << ":" << m_server->errorString();
        delete m_server;
        m_server = nullptr;
        return false;
    }

    m_port = m_server->serverPort();
    qDebug() << "MCP TCP: Listening on http://localhost:" << m_port << "/sse";
    return true;
}

void MCPTcpTransport::stop()
{
    // Close all SSE sessions
    for (auto it = m_sseSessions.begin(); it != m_sseSessions.end(); ++it)
    {
        it.value()->close();
    }
    m_sseSessions.clear();
    m_socketSessions.clear();
    m_pendingBuffers.clear();
    m_activeSessions.clear();

    if (m_server)
    {
        m_server->close();
        delete m_server;
        m_server = nullptr;
    }
}

void MCPTcpTransport::sendToSession(const QString &sessionId, const QJsonObject &message)
{
    auto it = m_sseSessions.find(sessionId);
    if (it == m_sseSessions.end() || !it.value()->isOpen())
        return;

    QTcpSocket *socket = it.value();
    QByteArray data = "data: " + QJsonDocument(message).toJson(QJsonDocument::Compact) + "\n\n";
    socket->write(data);
    socket->flush();
}

void MCPTcpTransport::broadcast(const QJsonObject &message)
{
    for (auto it = m_sseSessions.begin(); it != m_sseSessions.end(); ++it)
    {
        if (it.value()->isOpen())
        {
            QByteArray data = "data: " + QJsonDocument(message).toJson(QJsonDocument::Compact) + "\n\n";
            it.value()->write(data);
            it.value()->flush();
        }
    }
}

// ---- Connection handling ----

void MCPTcpTransport::onNewConnection()
{
    while (QTcpSocket *socket = m_server->nextPendingConnection())
    {
        connect(socket, &QTcpSocket::readyRead, this, &MCPTcpTransport::onReadyRead);
        connect(socket, &QTcpSocket::disconnected, this, &MCPTcpTransport::onDisconnected);
        m_pendingBuffers[socket] = QByteArray();
    }
}

void MCPTcpTransport::onReadyRead()
{
    QTcpSocket *socket = qobject_cast<QTcpSocket *>(sender());
    if (!socket) return;

    // If this socket is already an SSE session, just ignore raw reads
    if (m_socketSessions.contains(socket))
        return;

    m_pendingBuffers[socket].append(socket->readAll());

    // Check if we have a complete HTTP request (ends with \r\n\r\n)
    QByteArray &buf = m_pendingBuffers[socket];
    int headerEnd = buf.indexOf("\r\n\r\n");
    if (headerEnd < 0) return;

    QByteArray headerPart = buf.left(headerEnd);
    QByteArray bodyPart;
    int contentLength = 0;

    // Parse Content-Length
    for (const QByteArray &line : headerPart.split('\n'))
    {
        QByteArray trimmed = line.trimmed();
        if (trimmed.toLower().startsWith("content-length:"))
        {
            contentLength = trimmed.mid(15).trimmed().toInt();
        }
    }

    // Wait for full body if Content-Length is specified
    if (contentLength > 0)
    {
        QByteArray body = buf.mid(headerEnd + 4);
        if (body.size() < contentLength)
            return; // Wait for more data
        bodyPart = body.left(contentLength);
    }

    handleHttpRequest(socket, buf);
    m_pendingBuffers.remove(socket);
}

void MCPTcpTransport::onDisconnected()
{
    QTcpSocket *socket = qobject_cast<QTcpSocket *>(sender());
    if (!socket) return;

    QString sessionId = m_socketSessions.value(socket);
    if (!sessionId.isEmpty())
    {
        m_sseSessions.remove(sessionId);
        m_socketSessions.remove(socket);
        m_activeSessions.remove(sessionId);
    }
    m_pendingBuffers.remove(socket);
    socket->deleteLater();
}

// ---- HTTP handling ----

void MCPTcpTransport::handleHttpRequest(QTcpSocket *socket, const QByteArray &data)
{
    QString request = QString::fromUtf8(data);
    QStringList lines = request.split("\r\n");
    if (lines.isEmpty()) return;

    // Parse request line: METHOD PATH HTTP/1.1
    QStringList reqParts = lines[0].split(' ');
    if (reqParts.size() < 2) return;

    QString method = reqParts[0].toUpper();
    QString path = reqParts[1];

    // Find body (after \r\n\r\n)
    int bodyStart = data.indexOf("\r\n\r\n");
    QByteArray body;
    if (bodyStart > 0)
        body = data.mid(bodyStart + 4);

    // Parse query parameters
    QUrl url("http://localhost" + path);
    QString urlPath = url.path();
    QUrlQuery query(url);

    if (method == "GET" && urlPath == "/sse")
    {
        // Start SSE stream
        QString sessionId = generateSessionId();
        QByteArray response =
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/event-stream\r\n"
            "Cache-Control: no-cache\r\n"
            "Connection: keep-alive\r\n"
            "Access-Control-Allow-Origin: *\r\n"
            "\r\n";
        socket->write(response);
        socket->flush();

        // Send endpoint event with session URL
        QByteArray endpointData = "event: endpoint\r\n"
            "data: /message?sessionId=" + sessionId.toUtf8() + "\r\n\r\n";
        socket->write(endpointData);
        socket->flush();

        m_sseSessions[sessionId] = socket;
        m_socketSessions[socket] = sessionId;
        m_activeSessions.insert(sessionId);

        qDebug() << "MCP SSE: New session" << sessionId;
    }
    else if (method == "POST" && (urlPath == "/message" || urlPath == "/mcp"))
    {
        QString sessionId = query.queryItemValue("sessionId");
        if (sessionId.isEmpty())
        {
            // Direct POST /mcp mode (no SSE session required, respond directly)
            QJsonParseError err;
            QJsonDocument doc = QJsonDocument::fromJson(body, &err);
            if (err.error == QJsonParseError::NoError && doc.isObject())
            {
                emit messageReceived(doc.object(), QString());
            }
            // Send acknowledgment (actual response comes via broadcast or SSE)
            sendHttpResponse(socket, 202, "application/json",
                             "{\"jsonrpc\":\"2.0\",\"result\":{\"ack\":true}}");
        }
        else if (m_activeSessions.contains(sessionId))
        {
            QJsonParseError err;
            QJsonDocument doc = QJsonDocument::fromJson(body, &err);
            if (err.error == QJsonParseError::NoError && doc.isObject())
            {
                emit messageReceived(doc.object(), sessionId);
            }
            sendHttpResponse(socket, 202, "application/json",
                             "{\"jsonrpc\":\"2.0\",\"result\":{\"ack\":true}}");
        }
        else
        {
            sendHttpResponse(socket, 404, "application/json",
                             "{\"jsonrpc\":\"2.0\",\"error\":{\"code\":-32000,\"message\":\"Session not found\"}}");
        }
    }
    else if (method == "GET" && urlPath == "/health")
    {
        sendHttpResponse(socket, 200, "application/json",
                         "{\"status\":\"ok\",\"sessions\":" +
                         QByteArray::number(m_activeSessions.size()) + "}");
    }
    else if (method == "OPTIONS")
    {
        // CORS preflight
        QByteArray response =
            "HTTP/1.1 200 OK\r\n"
            "Access-Control-Allow-Origin: *\r\n"
            "Access-Control-Allow-Methods: GET, POST, OPTIONS\r\n"
            "Access-Control-Allow-Headers: Content-Type\r\n"
            "Content-Length: 0\r\n"
            "\r\n";
        socket->write(response);
        socket->flush();
    }
    else
    {
        sendHttpResponse(socket, 404, "text/plain", "Not Found");
    }
}

void MCPTcpTransport::sendHttpResponse(QTcpSocket *socket, int statusCode,
                                        const QString &contentType, const QByteArray &body)
{
    QString statusText = (statusCode == 200) ? "OK" :
                         (statusCode == 202) ? "Accepted" :
                         (statusCode == 404) ? "Not Found" : "Error";
    QByteArray response = "HTTP/1.1 " + QByteArray::number(statusCode) + " " +
                          statusText.toUtf8() + "\r\n" +
                          "Content-Type: " + contentType.toUtf8() + "\r\n" +
                          "Content-Length: " + QByteArray::number(body.size()) + "\r\n" +
                          "Access-Control-Allow-Origin: *\r\n" +
                          "\r\n" +
                          body;
    socket->write(response);
    socket->flush();

    // Close non-SSE connections after response
    if (!m_socketSessions.contains(socket))
    {
        socket->disconnectFromHost();
    }
}

QString MCPTcpTransport::generateSessionId()
{
    return QUuid::createUuid().toString(QUuid::WithoutBraces);
}
