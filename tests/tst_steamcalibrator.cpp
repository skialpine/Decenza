#include <QtTest>

#include "machine/steamcalibrator.h"

// Tests for SteamCalibrator stability analysis, dryness estimation, and sweep generation.
// Uses only static methods — no Settings/DE1Device needed.

class tst_SteamCalibrator : public QObject {
    Q_OBJECT

private:
    // Generate a stable pressure curve: constant value with small noise
    static QVector<QPointF> stablePressure(double meanBar = 3.0, double noise = 0.05,
                                           int samples = 150, double hz = 5.0) {
        QVector<QPointF> data;
        for (int i = 0; i < samples; i++) {
            double t = i / hz;
            double v = meanBar + noise * qSin(i * 0.7) * qCos(i * 1.3);
            data.append(QPointF(t, v));
        }
        return data;
    }

    // Generate an oscillating sawtooth pressure curve
    static QVector<QPointF> oscillatingPressure(double meanBar = 3.0, double amplitude = 0.8,
                                                 double freqHz = 3.0, int samples = 150,
                                                 double hz = 5.0) {
        QVector<QPointF> data;
        for (int i = 0; i < samples; i++) {
            double t = i / hz;
            double v = meanBar + amplitude * qSin(2.0 * M_PI * freqHz * t);
            data.append(QPointF(t, v));
        }
        return data;
    }

    // Generate a drifting pressure curve
    static QVector<QPointF> driftingPressure(double startBar = 2.5, double slope = 0.05,
                                              double noise = 0.03, int samples = 150,
                                              double hz = 5.0) {
        QVector<QPointF> data;
        for (int i = 0; i < samples; i++) {
            double t = i / hz;
            double v = startBar + slope * t + noise * qSin(i * 0.9);
            data.append(QPointF(t, v));
        }
        return data;
    }

private slots:

    // --- Stability analysis ---

    void stablePressureScoresHigh()
    {
        auto data = stablePressure(3.0, 0.05, 150, 5.0);
        auto result = SteamCalibrator::analyzeStability(data, 80, 160, 1500.0, 2.0);

        qDebug() << "Stable: score=" << result.stabilityScore << "cv=" << result.pressureCV
                 << "range=" << result.peakToPeakRange << "osc=" << result.oscillationRate
                 << "slope=" << result.pressureSlope;
        // Stable pressure should score significantly higher than oscillating
        auto oscData = oscillatingPressure(3.0, 0.8, 3.0, 150, 5.0);
        auto oscResult = SteamCalibrator::analyzeStability(oscData, 80, 160, 1500.0, 2.0);
        QVERIFY2(result.stabilityScore > oscResult.stabilityScore + 20.0,
                 qPrintable(QString("stable %1 should be >> oscillating %2")
                                .arg(result.stabilityScore).arg(oscResult.stabilityScore)));
        QVERIFY(result.pressureCV < 0.06);
        QVERIFY(result.peakToPeakRange < 0.5);
        QVERIFY(result.sampleCount > 0);
        QVERIFY(result.durationSeconds > 10.0);
    }

    void oscillatingPressureScoresLow()
    {
        auto data = oscillatingPressure(3.0, 0.8, 3.0, 150, 5.0);
        auto result = SteamCalibrator::analyzeStability(data, 80, 160, 1500.0, 2.0);

        QVERIFY(result.stabilityScore < 50.0);
        QVERIFY(result.pressureCV > 0.10);
        QVERIFY(result.oscillationRate > 2.0);
        QVERIFY(result.peakToPeakRange > 1.0);
    }

    void driftingPressureDetected()
    {
        auto data = driftingPressure(2.5, 0.05, 0.02, 150, 5.0);
        auto result = SteamCalibrator::analyzeStability(data, 80, 160, 1500.0, 2.0);

        qDebug() << "Drift: score=" << result.stabilityScore << "slope=" << result.pressureSlope
                 << "cv=" << result.pressureCV << "range=" << result.peakToPeakRange;
        QVERIFY(result.pressureSlope > 0.03);

        // Drift should score lower than a stable signal
        auto stableData = stablePressure(3.0, 0.05, 150, 5.0);
        auto stableResult = SteamCalibrator::analyzeStability(stableData, 80, 160, 1500.0, 2.0);
        QVERIFY2(result.stabilityScore < stableResult.stabilityScore,
                 qPrintable(QString("drift %1 should be < stable %2")
                                .arg(result.stabilityScore).arg(stableResult.stabilityScore)));
    }

    void trimSkipsEarlyData()
    {
        QVector<QPointF> data;
        for (int i = 0; i < 10; i++)
            data.append(QPointF(i / 5.0, 8.0));
        for (int i = 10; i < 160; i++)
            data.append(QPointF(i / 5.0, 3.0 + 0.02 * qSin(i)));

        auto result = SteamCalibrator::analyzeStability(data, 80, 160, 1500.0, 2.0);

        qDebug() << "Trim: score=" << result.stabilityScore << "avg=" << result.avgPressure;
        // After trimming the spike, the avg should be near 3.0 bar
        QVERIFY(result.avgPressure < 3.5);
        QVERIFY(result.avgPressure > 2.5);
        // And the score should be high (spike is trimmed away)
        QVERIFY2(result.stabilityScore >= 50.0,
                 qPrintable(QString("score %1").arg(result.stabilityScore)));
    }

    void tooShortReturnsLowSampleCount()
    {
        QVector<QPointF> data;
        for (int i = 0; i < 15; i++)
            data.append(QPointF(i / 5.0, 3.0));

        auto result = SteamCalibrator::analyzeStability(data, 80, 160, 1500.0, 2.0);
        QVERIFY(result.sampleCount < 10);
    }

    void emptyDataHandled()
    {
        QVector<QPointF> data;
        auto result = SteamCalibrator::analyzeStability(data, 80, 160, 1500.0, 2.0);
        QCOMPARE(result.sampleCount, 0);
        QCOMPARE(result.stabilityScore, 0.0);
    }

    void stabilityScoreMonotonic()
    {
        CalibrationStepResult step;
        step.avgPressure = 3.0;
        step.oscillationRate = 0.5;
        step.peakToPeakRange = 0.2;
        step.pressureSlope = 0.0;

        step.pressureCV = 0.02;
        double scoreGood = SteamCalibrator::computeStabilityScore(step);
        step.pressureCV = 0.10;
        double scoreMedium = SteamCalibrator::computeStabilityScore(step);
        step.pressureCV = 0.25;
        double scoreBad = SteamCalibrator::computeStabilityScore(step);

        QVERIFY(scoreGood > scoreMedium);
        QVERIFY(scoreMedium > scoreBad);
    }

    void stabilityScoreBounded()
    {
        CalibrationStepResult step;
        step.pressureCV = 10.0;
        step.oscillationRate = 100.0;
        step.peakToPeakRange = 50.0;
        step.pressureSlope = 5.0;

        double score = SteamCalibrator::computeStabilityScore(step);
        QVERIFY(score >= 0.0);
        QVERIFY(score <= 100.0);
    }

    // --- Dryness and dilution estimation ---

    void drynessFullAtLowFlow()
    {
        // DE1PRO (1500W) at 0.4 mL/s should fully vaporize
        double dryness = SteamCalibrator::estimateDryness(1500.0, 0.4, 160.0);
        QVERIFY2(dryness >= 0.99, qPrintable(QString("Expected ~1.0, got %1").arg(dryness)));
    }

    void drynessDropsAtHighFlow()
    {
        // DE1PRO (1500W) at 2.0 mL/s — heater can't keep up
        double dryness = SteamCalibrator::estimateDryness(1500.0, 2.0, 160.0);
        QVERIFY2(dryness < 0.5, qPrintable(QString("Expected <0.5, got %1").arg(dryness)));
    }

    void drynessHigherWithMorePower()
    {
        double drynessXXL = SteamCalibrator::estimateDryness(2200.0, 1.0, 160.0);
        double drynessPro = SteamCalibrator::estimateDryness(1500.0, 1.0, 160.0);
        QVERIFY(drynessXXL > drynessPro);
    }

    void dilutionMatchesDamianMath()
    {
        // Damian's calculation: 180g milk, 60°C rise, 224g pitcher → ~11.3% dilution
        // With perfectly dry steam (dryness = 1.0)
        double dilution = SteamCalibrator::estimateDilution(1.0, 180.0, 60.0, 224.0);
        // Should be close to 11.3% (Damian's theoretical minimum)
        QVERIFY2(dilution > 10.0, qPrintable(QString("Expected >10%, got %1%").arg(dilution)));
        QVERIFY2(dilution < 13.0, qPrintable(QString("Expected <13%, got %1%").arg(dilution)));
    }

    void dilutionHigherWithWetSteam()
    {
        double dilutionDry = SteamCalibrator::estimateDilution(1.0);
        double dilutionWet = SteamCalibrator::estimateDilution(0.7);
        QVERIFY(dilutionWet > dilutionDry);
    }

    void dilutionReasonableRange()
    {
        // Standard case: 180g milk, 55°C rise
        double dilution = SteamCalibrator::estimateDilution(1.0, 180.0, 55.0, 224.0);
        // Should be in the 9-12% range for dry steam
        QVERIFY2(dilution > 8.0, qPrintable(QString("Expected >8%, got %1%").arg(dilution)));
        QVERIFY2(dilution < 14.0, qPrintable(QString("Expected <14%, got %1%").arg(dilution)));
    }

    void analysisIncludesDrynessEstimate()
    {
        auto data = stablePressure(3.0, 0.05, 150, 5.0);
        // DE1PRO at 0.8 mL/s — should be near sweet spot
        auto result = SteamCalibrator::analyzeStability(data, 80, 160, 1500.0, 2.0);

        QVERIFY(result.estimatedDryness > 0.0);
        QVERIFY(result.estimatedDryness <= 1.0);
        QVERIFY(result.estimatedDilution > 5.0);
        QVERIFY(result.estimatedDilution < 30.0);
    }

    // --- Sweep generation ---

    void sweepGenerationProModel()
    {
        auto steps = SteamCalibrator::generateFlowSweep(3);
        QVERIFY(steps.size() >= 4);
        QVERIFY(steps.first() >= 40);
        QVERIFY(steps.last() <= 160);
        for (qsizetype i = 1; i < steps.size(); i++)
            QVERIFY(steps[i] > steps[i - 1]);
    }

    void sweepGenerationXXLModel()
    {
        auto steps = SteamCalibrator::generateFlowSweep(6);
        QVERIFY(steps.size() >= 4);
        QVERIFY(steps.first() >= 50);
        QVERIFY(steps.last() <= 200);
    }

    void sweepGeneration110VNarrower()
    {
        auto steps120 = SteamCalibrator::generateFlowSweep(3, 120);
        auto steps220 = SteamCalibrator::generateFlowSweep(3, 220);
        QVERIFY(steps120.last() <= steps220.last());
    }

    void tempSweepHasThreeValues()
    {
        auto temps = SteamCalibrator::generateTempSweep();
        QCOMPARE(temps.size(), 3);
        QVERIFY(temps.first() < temps.last());
    }

    // --- Heater wattage ---

    void heaterWattsKnownModels()
    {
        QCOMPARE(SteamCalibrator::heaterWattsForModel(3), 1500.0);   // PRO
        QCOMPARE(SteamCalibrator::heaterWattsForModel(6), 2200.0);   // XXL
        QCOMPARE(SteamCalibrator::heaterWattsForModel(7), 3000.0);   // Bengle
    }

    void heaterWattsReducedAt110V()
    {
        double watts220 = SteamCalibrator::heaterWattsForModel(3, 220);
        double watts110 = SteamCalibrator::heaterWattsForModel(3, 110);
        QVERIFY(watts110 < watts220);
    }
};

QTEST_MAIN(tst_SteamCalibrator)
#include "tst_steamcalibrator.moc"
