#pragma once

#include <QObject>
#include <QTimer>
#include <QVector>
#include <QJsonObject>
#include <QJsonArray>
#include <QElapsedTimer>

struct MemorySample {
    qint64 timestampMs;
    quint64 rssBytes;
    int qobjectCount;
};

class MemoryMonitor : public QObject {
    Q_OBJECT

    Q_PROPERTY(double currentRssMB READ currentRssMB NOTIFY sampleTaken)
    Q_PROPERTY(double peakRssMB READ peakRssMB NOTIFY sampleTaken)
    Q_PROPERTY(int qobjectCount READ qobjectCount NOTIFY sampleTaken)

public:
    explicit MemoryMonitor(QObject* parent = nullptr);

    double currentRssMB() const;
    double peakRssMB() const;
    double startupRssMB() const;
    int qobjectCount() const { return m_lastQObjectCount; }
    quint64 currentRssBytes() const { return m_lastRss; }
    quint64 peakRssBytes() const { return m_peakRss; }
    quint64 startupRssBytes() const { return m_startupRss; }

    QJsonObject toJson() const;

signals:
    void sampleTaken();

private slots:
    void takeSample();

private:
    quint64 readRss() const;
    int countQObjects() const;

    QTimer m_timer;
    QElapsedTimer m_uptime;

    QVector<MemorySample> m_samples;
    static constexpr int MAX_SAMPLES = 1440;  // 24 hours at 60s interval

    quint64 m_lastRss = 0;
    quint64 m_peakRss = 0;
    quint64 m_startupRss = 0;
    int m_lastQObjectCount = 0;
    bool m_firstSample = true;
};
