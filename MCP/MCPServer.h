#ifndef MCPSERVER_H
#define MCPSERVER_H

#include <QObject>
#include <QJsonObject>
#include <QJsonArray>
#include <functional>

#include "MCPStdioTransport.h"
#include "MCPTcpTransport.h"

struct MCPToolDefinition
{
    QString name;
    QString description;
    QJsonObject inputSchema;
};

/// Main MCP server: JSON-RPC 2.0 dispatcher with tool registry.
/// Lives on the main thread. Supports both stdio and TCP/SSE transports.
class MCPServer : public QObject
{
    Q_OBJECT
public:
    explicit MCPServer(QObject *parent = nullptr);
    ~MCPServer() override;

    /// Start listening on stdio for MCP JSON-RPC messages.
    void start();

    /// Start listening on a TCP port (SSE+HTTP transport).
    /// Returns true if successful, false if port is in use.
    bool startTcp(quint16 port = 9876);

    /// Stop all transports.
    void stop();

    /// Stop only the TCP transport (keeps stdio alive if active).
    void stopTcp();

    /// Get the TCP port (0 if TCP transport not active).
    quint16 tcpPort() const;

    /// Register a tool handler. handler receives (QJsonObject arguments) and returns QJsonObject result.
    using ToolHandler = std::function<QJsonObject(const QJsonObject &arguments)>;
    void registerTool(const QString &name, const QString &description,
                      const QJsonObject &inputSchema, ToolHandler handler);

private slots:
    // Stdio transport
    void onMessageReceived(const QJsonObject &message);
    // TCP transport (session-aware)
    void onTcpMessageReceived(const QJsonObject &message, const QString &sessionId);

private:
    void sendResponse(const QJsonValue &id, const QJsonObject &result,
                      const QString &sessionId = QString());
    void sendError(const QJsonValue &id, int code, const QString &message,
                   const QString &sessionId = QString());
    void handleRequest(const QJsonValue &id, const QString &method,
                       const QJsonObject &params, const QString &sessionId = QString());

    QJsonObject handleInitialize(const QJsonObject &params);
    QJsonObject handleToolsList();
    QJsonObject handleToolsCall(const QJsonObject &params);
    QJsonObject handleResourcesList();
    QJsonObject handleResourcesRead(const QJsonObject &params);

    MCPStdioTransport *m_transport;
    MCPTcpTransport *m_tcpTransport;
    bool m_initialized;

    struct ToolEntry
    {
        QString name;
        QString description;
        QJsonObject inputSchema;
        ToolHandler handler;
    };
    QList<ToolEntry> m_tools;

    QJsonObject m_serverCapabilities;
};

#endif // MCPSERVER_H
