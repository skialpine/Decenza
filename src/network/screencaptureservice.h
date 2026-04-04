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
    void onCaptureTimer();

private:
    static constexpr int kTileSize = 64;
    static constexpr int kMaxMessageSize = 22000; // ~29KB after base64+JSON, under 32KB API Gateway limit
    static constexpr int kCaptureIntervalMs = 500; // ~2fps

    void captureAndSend();
    QByteArray encodeTile(const QImage& image, int x, int y, int w, int h);
    void sendTiles(const QVector<QPair<int,int>>& changedTiles, const QImage& frame);

    QQuickWindow* m_window;
    QWebSocket* m_socket;
    double m_scaleFactor;
    QImage m_previousFrame;
    QTimer m_captureTimer;
    QElapsedTimer m_byteCounterTimer;
    qint64 m_bytesSentThisSecond = 0;
    bool m_initialFrameSent = false;
};
