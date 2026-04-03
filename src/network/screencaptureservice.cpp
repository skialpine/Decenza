#include "network/screencaptureservice.h"

#include <QQuickWindow>
#include <QWebSocket>
#include <QBuffer>
#include <QDebug>
#include <QtGui/qpa/qwindowsysteminterface.h>

ScreenCaptureService::ScreenCaptureService(QQuickWindow* window, QWebSocket* socket,
                                           double scaleFactor, QObject* parent)
    : QObject(parent)
    , m_window(window)
    , m_socket(socket)
    , m_scaleFactor(qBound(0.1, scaleFactor, 1.0))
{
    connect(m_window, &QQuickWindow::frameSwapped,
            this, &ScreenCaptureService::onFrameSwapped);

    m_heartbeatTimer.setInterval(kHeartbeatMs);
    connect(&m_heartbeatTimer, &QTimer::timeout,
            this, &ScreenCaptureService::onHeartbeatTimer);
    m_heartbeatTimer.start();

    m_throttleTimer.start();

    qDebug() << "ScreenCaptureService: started, scale:" << m_scaleFactor;
}

ScreenCaptureService::~ScreenCaptureService()
{
    qDebug() << "ScreenCaptureService: stopped";
}

void ScreenCaptureService::onFrameSwapped()
{
    if (m_captureScheduled) return;
    m_captureScheduled = true;

    if (m_throttleTimer.elapsed() >= 1000) {
        m_bytesSentThisSecond = 0;
        m_throttleTimer.restart();
    }
    if (m_bytesSentThisSecond > 500000) return;

    QMetaObject::invokeMethod(this, &ScreenCaptureService::captureAndSend,
                              Qt::QueuedConnection);
}

void ScreenCaptureService::onHeartbeatTimer()
{
    captureAndSend();
}

void ScreenCaptureService::captureAndSend()
{
    m_captureScheduled = false;

    QImage frame = m_window->grabWindow();
    if (frame.isNull()) return;

    int scaledW = static_cast<int>(frame.width() * m_scaleFactor);
    int scaledH = static_cast<int>(frame.height() * m_scaleFactor);
    QImage scaled = frame.scaled(scaledW, scaledH, Qt::IgnoreAspectRatio,
                                  Qt::SmoothTransformation);
    scaled = scaled.convertToFormat(QImage::Format_RGB32);

    int cols = (scaled.width() + kTileSize - 1) / kTileSize;
    int rows = (scaled.height() + kTileSize - 1) / kTileSize;

    QVector<QPair<int,int>> changedTiles;

    for (int ty = 0; ty < rows; ++ty) {
        for (int tx = 0; tx < cols; ++tx) {
            int x = tx * kTileSize;
            int y = ty * kTileSize;
            int w = qMin(kTileSize, scaled.width() - x);
            int h = qMin(kTileSize, scaled.height() - y);

            bool changed = !m_initialFrameSent;

            if (!changed && !m_previousFrame.isNull()) {
                for (int py = y; py < y + h && !changed; ++py) {
                    const QRgb* newLine = reinterpret_cast<const QRgb*>(scaled.scanLine(py));
                    const QRgb* oldLine = reinterpret_cast<const QRgb*>(m_previousFrame.scanLine(py));
                    for (int px = x; px < x + w; ++px) {
                        if (newLine[px] != oldLine[px]) {
                            changed = true;
                            break;
                        }
                    }
                }
            }

            if (changed) {
                changedTiles.append({tx, ty});
            }
        }
    }

    if (!changedTiles.isEmpty()) {
        sendTiles(changedTiles, scaled);
        m_heartbeatTimer.start();
    }

    m_previousFrame = scaled;
    m_initialFrameSent = true;
}

QByteArray ScreenCaptureService::encodeTile(const QImage& image, int x, int y, int w, int h)
{
    QImage tile = image.copy(x, y, w, h);
    QByteArray data;
    QBuffer buffer(&data);
    buffer.open(QIODevice::WriteOnly);
    tile.save(&buffer, "WEBP", 80);
    return data;
}

void ScreenCaptureService::sendTiles(const QVector<QPair<int,int>>& changedTiles,
                                      const QImage& frame)
{
    int idx = 0;
    while (idx < changedTiles.size()) {
        QByteArray msg;
        msg.reserve(kMaxMessageSize);

        msg.append(static_cast<char>(0x01));
        quint16 w = static_cast<quint16>(frame.width());
        quint16 h = static_cast<quint16>(frame.height());
        msg.append(static_cast<char>((w >> 8) & 0xFF));
        msg.append(static_cast<char>(w & 0xFF));
        msg.append(static_cast<char>((h >> 8) & 0xFF));
        msg.append(static_cast<char>(h & 0xFF));
        msg.append(static_cast<char>(kTileSize));

        int countPos = msg.size();
        msg.append(static_cast<char>(0));

        int tileCount = 0;

        while (idx < changedTiles.size()) {
            auto [tx, ty] = changedTiles[idx];
            int x = tx * kTileSize;
            int y = ty * kTileSize;
            int tw = qMin(kTileSize, frame.width() - x);
            int th = qMin(kTileSize, frame.height() - y);

            QByteArray tileData = encodeTile(frame, x, y, tw, th);

            if (msg.size() + 4 + tileData.size() > kMaxMessageSize && tileCount > 0) {
                break;
            }

            msg.append(static_cast<char>(tx));
            msg.append(static_cast<char>(ty));
            quint16 len = static_cast<quint16>(tileData.size());
            msg.append(static_cast<char>((len >> 8) & 0xFF));
            msg.append(static_cast<char>(len & 0xFF));
            msg.append(tileData);

            tileCount++;
            idx++;
        }

        msg[countPos] = static_cast<char>(tileCount);
        m_socket->sendBinaryMessage(msg);
        m_bytesSentThisSecond += msg.size();

        qDebug() << "ScreenCaptureService: sent" << tileCount << "tiles,"
                 << msg.size() << "bytes";
    }
}

void ScreenCaptureService::handleTouchEvent(const QByteArray& data)
{
    if (data.size() < 7) return;

    quint8 touchType = static_cast<quint8>(data[1]);
    quint16 normX = (static_cast<quint8>(data[2]) << 8) | static_cast<quint8>(data[3]);
    quint16 normY = (static_cast<quint8>(data[4]) << 8) | static_cast<quint8>(data[5]);
    quint8 pointId = static_cast<quint8>(data[6]);

    qreal x = (normX / 65535.0) * m_window->width();
    qreal y = (normY / 65535.0) * m_window->height();

    QPointF pos(x, y);

    QEvent::Type eventType;
    switch (touchType) {
    case 0: eventType = QEvent::MouseButtonPress; break;
    case 1: eventType = QEvent::MouseMove; break;
    case 2: eventType = QEvent::MouseButtonRelease; break;
    default: return;
    }

    Q_UNUSED(pointId)

    Qt::MouseButton button = (eventType == QEvent::MouseMove) ? Qt::NoButton : Qt::LeftButton;
    Qt::MouseButtons buttons = (eventType == QEvent::MouseButtonRelease) ? Qt::NoButton : Qt::LeftButton;

    QWindowSystemInterface::handleMouseEvent(
        m_window, pos, m_window->mapToGlobal(pos),
        buttons, button, eventType);
}
