#pragma once

#include <QObject>
#include <QString>
#include <QJsonObject>
#include <QJsonArray>
#include <QHash>
#include <functional>

// Tool handler callback: takes arguments JSON, returns result JSON.
// For async tools, the handler should return a "pending" marker and emit
// the result later via McpServer's response mechanism.
using McpToolHandler = std::function<QJsonObject(const QJsonObject& arguments)>;

struct McpToolDefinition {
    QString name;
    QString description;
    QJsonObject inputSchema;    // JSON Schema for the tool's parameters
    McpToolHandler handler;
    QString category;           // "read", "control", or "settings"
};

class McpToolRegistry : public QObject {
    Q_OBJECT
public:
    explicit McpToolRegistry(QObject* parent = nullptr) : QObject(parent) {}

    void registerTool(const QString& name, const QString& description,
                      const QJsonObject& inputSchema, McpToolHandler handler,
                      const QString& category)
    {
        McpToolDefinition tool;
        tool.name = name;
        tool.description = description;
        tool.inputSchema = inputSchema;
        tool.handler = handler;
        tool.category = category;
        m_tools[name] = tool;
    }

    // List all tools. Tools above the current access level are still listed
    // (so the AI knows they exist) but their descriptions note the required level.
    // Access is enforced in callTool — restricted tools return an error when called.
    // 0 = Monitor (read only), 1 = Control (read + control), 2 = Full (all)
    QJsonArray listTools(int accessLevel) const
    {
        static const char* levelNames[] = {"Monitor", "Control", "Full"};
        QJsonArray result;
        for (auto it = m_tools.constBegin(); it != m_tools.constEnd(); ++it) {
            const auto& tool = it.value();
            int required = categoryMinLevel(tool.category);

            QJsonObject toolJson;
            toolJson["name"] = tool.name;
            if (required > accessLevel) {
                int reqClamped = qBound(0, required, 2);
                toolJson["description"] = QString("[DISABLED — requires '%1' access level in Settings > AI > MCP] ")
                    .arg(levelNames[reqClamped]) + tool.description;
            } else {
                toolJson["description"] = tool.description;
            }
            toolJson["inputSchema"] = tool.inputSchema;
            result.append(toolJson);
        }
        return result;
    }

    // Call a tool, checking access level.
    // Arguments are normalized against the tool's input schema before dispatch —
    // MCP clients may send integers as strings (especially after the confirmation
    // round-trip where args are serialized to JSON text and re-parsed).
    QJsonObject callTool(const QString& name, const QJsonObject& arguments,
                         int accessLevel, QString& errorOut) const
    {
        auto it = m_tools.constFind(name);
        if (it == m_tools.constEnd()) {
            errorOut = "Unknown tool: " + name;
            return {};
        }
        const auto& tool = it.value();
        if (categoryMinLevel(tool.category) > accessLevel) {
            errorOut = "Access level insufficient";
            return {};
        }
        return tool.handler(normalizeArguments(arguments, tool.inputSchema));
    }

    bool hasTool(const QString& name) const { return m_tools.contains(name); }

    // Returns the category of a tool ("read", "control", "settings") or empty string
    QString toolCategory(const QString& name) const
    {
        auto it = m_tools.constFind(name);
        return (it != m_tools.constEnd()) ? it.value().category : QString();
    }

private:
    // Coerce string-typed values to the type declared in the tool's inputSchema.
    // MCP clients may send "123" instead of 123 after a confirmation round-trip.
    static QJsonObject normalizeArguments(const QJsonObject& args, const QJsonObject& schema)
    {
        QJsonObject properties = schema["properties"].toObject();
        if (properties.isEmpty()) return args;

        QJsonObject normalized = args;
        for (auto it = args.begin(); it != args.end(); ++it) {
            if (!it.value().isString()) continue;  // only coerce strings
            QJsonObject prop = properties[it.key()].toObject();
            QString type = prop["type"].toString();
            if (type == "integer") {
                bool ok;
                qint64 v = it.value().toString().toLongLong(&ok);
                if (ok) normalized[it.key()] = v;
            } else if (type == "number") {
                bool ok;
                double v = it.value().toString().toDouble(&ok);
                if (ok) normalized[it.key()] = v;
            } else if (type == "boolean") {
                QString s = it.value().toString().toLower();
                if (s == "true") normalized[it.key()] = true;
                else if (s == "false") normalized[it.key()] = false;
            }
        }
        return normalized;
    }

    static int categoryMinLevel(const QString& category)
    {
        if (category == "read") return 0;
        if (category == "control") return 1;
        if (category == "settings") return 2;
        return 3; // unknown category — deny
    }

    QHash<QString, McpToolDefinition> m_tools;
};
