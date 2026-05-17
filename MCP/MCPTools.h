#ifndef MCPTOOLS_H
#define MCPTOOLS_H

#include "MCPServer.h"

/// Register all WL4Editor MCP tools on the given server.
/// Must be called after a ROM is loaded (tools access WL4Editor state).
void MCP_RegisterAllTools(MCPServer *server);

#endif // MCPTOOLS_H
