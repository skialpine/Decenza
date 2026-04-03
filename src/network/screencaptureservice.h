#pragma once

#include <QObject>
#include <QImage>
#include <QTimer>
#include <QElapsedTimer>
#include <QVector>

class QQuickWindow;
class QWebSocket;

class ScreenCaptureService : public QObject {
    Q_OBJECT

public:
    explicit ScreenCaptureService(QQuickWindow* window, QWebSocket* socket,
                                   double scaleFactor = 0.5, QObject* parent = nullptr);
    ~ScreenCaptureService();

    void handleTouchEvent(const QByteArray& data);

private slots:
    void onFrameSwapped();
    void onHeartbeatTimer();

private:
    static constexpr int kTileSize = 64;
    static constexpr int kMaxMessageSize = 120000;
    static constexpr int kHeartbeatMs = 30000;

    void captureAndSend();
    QByteArray encodeTile(const QImage& image, int x, int y, int w, int h);
    void sendTiles(const QVector<QPair<int,int>>& changedTiles, const QImage& frame);

    QQuickWindow* m_window;
    QWebSocket* m_socket;
    double m_scaleFactor;
    QImage m_previousFrame;
    QTimer m_heartbeatTimer;
    QElapsedTimer m_throttleTimer;
    qint64 m_bytesSentThisSecond = 0;
    bool m_captureScheduled = false;
    bool m_initialFrameSent = false;
};
