#include "MCPServer.h"

#include <QJsonDocument>
#include <QJsonArray>

MCPServer::MCPServer(QObject *parent)
    : QObject(parent), m_transport(nullptr), m_tcpTransport(nullptr), m_initialized(false)
{
}

MCPServer::~MCPServer()
{
    stop();
}

void MCPServer::start()
{
    if (m_transport) return;

    m_transport = new MCPStdioTransport(this);
    connect(m_transport, &MCPStdioTransport::messageReceived,
            this, &MCPServer::onMessageReceived);

    // Build server capabilities
    m_serverCapabilities["protocolVersion"] = QString("2024-11-05");
    QJsonObject caps;
    caps["tools"] = QJsonObject();
    caps["resources"] = QJsonObject();
    m_serverCapabilities["capabilities"] = caps;
    m_serverCapabilities["serverInfo"] = QJsonObject{
        {"name", QString("wl4editor-mcp")},
        {"version", QString("1.0.0")}
    };

    m_transport->start();
}

bool MCPServer::startTcp(quint16 port)
{
    if (m_tcpTransport) return true; // Already running

    m_tcpTransport = new MCPTcpTransport(this);
    connect(m_tcpTransport, &MCPTcpTransport::messageReceived,
            this, &MCPServer::onTcpMessageReceived);

    if (!m_tcpTransport->start(port))
    {
        delete m_tcpTransport;
        m_tcpTransport = nullptr;
        return false;
    }

    // Ensure capabilities are built (in case stdio wasn't started first)
    if (m_serverCapabilities.isEmpty())
    {
        m_serverCapabilities["protocolVersion"] = QString("2024-11-05");
        QJsonObject caps;
        caps["tools"] = QJsonObject();
        caps["resources"] = QJsonObject();
        m_serverCapabilities["capabilities"] = caps;
        m_serverCapabilities["serverInfo"] = QJsonObject{
            {"name", QString("wl4editor-mcp")},
            {"version", QString("1.0.0")}
        };
    }

    return true;
}

quint16 MCPServer::tcpPort() const
{
    return m_tcpTransport ? m_tcpTransport->port() : 0;
}

void MCPServer::stop()
{
    if (m_transport)
    {
        m_transport->stop();
        m_transport->deleteLater();
        m_transport = nullptr;
    }
    stopTcp();
    m_initialized = false;
}

void MCPServer::stopTcp()
{
    if (m_tcpTransport)
    {
        m_tcpTransport->stop();
        m_tcpTransport->deleteLater();
        m_tcpTransport = nullptr;
    }
}

void MCPServer::registerTool(const QString &name, const QString &description,
                              const QJsonObject &inputSchema, ToolHandler handler)
{
    m_tools.append({name, description, inputSchema, handler});
}

// ---- Message handling (stdio) ----

void MCPServer::onMessageReceived(const QJsonObject &message)
{
    if (message.value("jsonrpc").toString() != "2.0")
        return;

    QJsonValue id = message.value("id");
    QString method = message.value("method").toString();

    if (!method.isEmpty())
    {
        QJsonObject params = message.value("params").toObject();
        handleRequest(id, method, params, QString());
    }
}

// ---- Message handling (TCP) ----

void MCPServer::onTcpMessageReceived(const QJsonObject &message, const QString &sessionId)
{
    if (message.value("jsonrpc").toString() != "2.0")
        return;

    QJsonValue id = message.value("id");
    QString method = message.value("method").toString();

    if (!method.isEmpty())
    {
        QJsonObject params = message.value("params").toObject();
        handleRequest(id, method, params, sessionId);
    }
}

// ---- Request dispatch ----

void MCPServer::handleRequest(const QJsonValue &id, const QString &method,
                               const QJsonObject &params, const QString &sessionId)
{
    QJsonObject result;

    if (method == "initialize")
    {
        result = handleInitialize(params);
    }
    else if (method == "notifications/initialized")
    {
        m_initialized = true;
        if (id.isUndefined()) return;
        result = QJsonObject();
    }
    else if (method == "tools/list")
    {
        result = handleToolsList();
    }
    else if (method == "tools/call")
    {
        result = handleToolsCall(params);
    }
    else if (method == "resources/list")
    {
        result = handleResourcesList();
    }
    else if (method == "resources/read")
    {
        result = handleResourcesRead(params);
    }
    else if (method == "ping")
    {
        result = QJsonObject();
    }
    else
    {
        if (!id.isUndefined())
            sendError(id, -32601, "Method not found: " + method, sessionId);
        return;
    }

    if (!id.isUndefined())
        sendResponse(id, result, sessionId);
}

// ---- Response sending ----

void MCPServer::sendResponse(const QJsonValue &id, const QJsonObject &result,
                              const QString &sessionId)
{
    QJsonObject response;
    response["jsonrpc"] = QString("2.0");
    response["id"] = id;
    response["result"] = result;

    if (!sessionId.isEmpty() && m_tcpTransport)
    {
        m_tcpTransport->sendToSession(sessionId, response);
    }
    else if (m_transport)
    {
        m_transport->send(response);
    }
}

void MCPServer::sendError(const QJsonValue &id, int code, const QString &message,
                           const QString &sessionId)
{
    QJsonObject response;
    response["jsonrpc"] = QString("2.0");
    response["id"] = id;
    QJsonObject err;
    err["code"] = code;
    err["message"] = message;
    response["error"] = err;

    if (!sessionId.isEmpty() && m_tcpTransport)
    {
        m_tcpTransport->sendToSession(sessionId, response);
    }
    else if (m_transport)
    {
        m_transport->send(response);
    }
}

// ---- Protocol handlers ----

QJsonObject MCPServer::handleInitialize(const QJsonObject &params)
{
    Q_UNUSED(params);
    return m_serverCapabilities;
}

QJsonObject MCPServer::handleToolsList()
{
    QJsonArray tools;
    for (const auto &t : m_tools)
    {
        QJsonObject tool;
        tool["name"] = t.name;
        tool["description"] = t.description;
        tool["inputSchema"] = t.inputSchema;
        tools.append(tool);
    }
    QJsonObject result;
    result["tools"] = tools;
    return result;
}

QJsonObject MCPServer::handleToolsCall(const QJsonObject &params)
{
    QString toolName = params.value("name").toString();
    QJsonObject arguments = params.value("arguments").toObject();

    for (const auto &t : m_tools)
    {
        if (t.name == toolName)
        {
            QJsonObject toolResult;
            try
            {
                toolResult = t.handler(arguments);
            }
            catch (...)
            {
                QJsonObject errorResult;
                QJsonObject content;
                content["type"] = QString("text");
                content["text"] = QString("Tool execution threw an exception");
                errorResult["content"] = QJsonArray{content};
                errorResult["isError"] = true;
                return errorResult;
            }
            return toolResult;
        }
    }

    QJsonObject errorResult;
    QJsonObject content;
    content["type"] = QString("text");
    content["text"] = QString("Tool not found: %1").arg(toolName);
    errorResult["content"] = QJsonArray{content};
    errorResult["isError"] = true;
    return errorResult;
}

QJsonObject MCPServer::handleResourcesList()
{
    QJsonArray resources;
    auto addResource = [&](const QString &uri, const QString &name,
                           const QString &desc)
    {
        QJsonObject r;
        r["uri"] = uri;
        r["name"] = name;
        r["description"] = desc;
        r["mimeType"] = QString("application/json");
        resources.append(r);
    };

    addResource("wl4://room/current", "Current Room", "Current room in WL4Editor");
    addResource("wl4://level/current", "Current Level", "Current level in WL4Editor");
    addResource("wl4://room/list", "Room List", "All rooms in current level");

    QJsonObject result;
    result["resources"] = resources;
    return result;
}

QJsonObject MCPServer::handleResourcesRead(const QJsonObject &params)
{
    QString uri = params.value("uri").toString();
    QJsonObject result;
    QJsonArray contents;
    QJsonObject content;
    content["uri"] = uri;
    content["mimeType"] = QString("application/json");
    content["text"] = QString("Resource not available: %1").arg(uri);
    contents.append(content);
    result["contents"] = contents;
    return result;
}
