#include "shotsummarizer.h"
#include "../models/shotdatamodel.h"
#include "../profile/profile.h"
#include "../network/visualizeruploader.h"

#include <cmath>
#include <algorithm>

ShotSummarizer::ShotSummarizer(QObject* parent)
    : QObject(parent)
{
}

ShotSummary ShotSummarizer::summarize(const ShotDataModel* shotData,
                                       const Profile* profile,
                                       const ShotMetadata& metadata,
                                       double doseWeight,
                                       double finalWeight) const
{
    ShotSummary summary;

    if (!shotData) {
        return summary;
    }

    // Profile info
    if (profile) {
        summary.profileTitle = profile->title();
        summary.profileType = profile->mode() == Profile::Mode::FrameBased ? "Frame-based" : "Direct Control";
    }

    // Get the data vectors
    const auto& pressureData = shotData->pressureData();
    const auto& flowData = shotData->flowData();
    const auto& tempData = shotData->temperatureData();
    const auto& weightData = shotData->weightData();

    if (pressureData.isEmpty()) {
        return summary;
    }

    // Store raw curve data for detailed AI analysis
    summary.pressureCurve = pressureData;
    summary.flowCurve = flowData;
    summary.tempCurve = tempData;
    summary.weightCurve = weightData;

    // Store target/goal curves (what the profile intended)
    summary.pressureGoalCurve = shotData->pressureGoalData();
    summary.flowGoalCurve = shotData->flowGoalData();
    summary.tempGoalCurve = shotData->temperatureGoalData();

    // Overall metrics
    summary.totalDuration = pressureData.last().x();
    summary.doseWeight = doseWeight;
    summary.finalWeight = finalWeight;
    summary.ratio = doseWeight > 0 ? finalWeight / doseWeight : 0;

    // DYE metadata
    summary.beanBrand = metadata.beanBrand;
    summary.beanType = metadata.beanType;
    summary.roastDate = metadata.roastDate;
    summary.roastLevel = metadata.roastLevel;
    summary.grinderModel = metadata.grinderModel;
    summary.grinderSetting = metadata.grinderSetting;
    summary.enjoymentScore = metadata.espressoEnjoyment;
    summary.tastingNotes = metadata.espressoNotes;

    // Extraction indicators
    summary.timeToFirstDrip = findTimeToFirstDrip(flowData);
    // Channeling detection will be done after phase processing (see below)

    // Temperature stability check - compare actual vs TARGET (not just variance)
    // A declining temperature profile is intentional, not "unstable"
    const auto& tempGoalData = shotData->temperatureGoalData();
    if (!tempGoalData.isEmpty()) {
        // Calculate average deviation from target
        double deviationSum = 0;
        int count = 0;
        for (const auto& point : tempData) {
            double target = findValueAtTime(tempGoalData, point.x());
            if (target > 0) {
                deviationSum += std::abs(point.y() - target);
                count++;
            }
        }
        double avgDeviation = count > 0 ? deviationSum / count : 0;
        summary.temperatureUnstable = avgDeviation > 2.0;  // >2°C average deviation from target
    } else {
        // No target data - fall back to variance check
        double tempStdDev = calculateStdDev(tempData, 0, summary.totalDuration);
        summary.temperatureUnstable = tempStdDev > 2.0;
    }

    // Get phase markers from shot data
    QVariantList markers = shotData->phaseMarkersVariant();

    if (markers.isEmpty()) {
        // No markers - create a single "Extraction" phase
        PhaseSummary phase;
        phase.name = "Extraction";
        phase.startTime = 0;
        phase.endTime = summary.totalDuration;
        phase.duration = summary.totalDuration;

        phase.avgPressure = calculateAverage(pressureData, 0, summary.totalDuration);
        phase.maxPressure = calculateMax(pressureData, 0, summary.totalDuration);
        phase.minPressure = calculateMin(pressureData, 0, summary.totalDuration);
        phase.pressureAtStart = findValueAtTime(pressureData, 0);
        phase.pressureAtMiddle = findValueAtTime(pressureData, summary.totalDuration / 2);
        phase.pressureAtEnd = findValueAtTime(pressureData, summary.totalDuration);

        phase.avgFlow = calculateAverage(flowData, 0, summary.totalDuration);
        phase.maxFlow = calculateMax(flowData, 0, summary.totalDuration);
        phase.minFlow = calculateMin(flowData, 0, summary.totalDuration);
        phase.flowAtStart = findValueAtTime(flowData, 0);
        phase.flowAtMiddle = findValueAtTime(flowData, summary.totalDuration / 2);
        phase.flowAtEnd = findValueAtTime(flowData, summary.totalDuration);

        phase.avgTemperature = calculateAverage(tempData, 0, summary.totalDuration);
        phase.tempStability = calculateStdDev(tempData, 0, summary.totalDuration);

        double startWeight = findValueAtTime(weightData, 0);
        double endWeight = findValueAtTime(weightData, summary.totalDuration);
        phase.weightGained = endWeight - startWeight;

        summary.phases.append(phase);
    } else {
        // Process each phase from markers
        for (int i = 0; i < markers.size(); i++) {
            QVariantMap marker = markers[i].toMap();
            double startTime = marker["time"].toDouble();
            double endTime = (i + 1 < markers.size())
                ? markers[i + 1].toMap()["time"].toDouble()
                : summary.totalDuration;

            if (endTime <= startTime) continue;

            PhaseSummary phase;
            phase.name = marker["label"].toString();
            phase.startTime = startTime;
            phase.endTime = endTime;
            phase.duration = endTime - startTime;

            // Pressure metrics
            phase.avgPressure = calculateAverage(pressureData, startTime, endTime);
            phase.maxPressure = calculateMax(pressureData, startTime, endTime);
            phase.minPressure = calculateMin(pressureData, startTime, endTime);
            phase.pressureAtStart = findValueAtTime(pressureData, startTime);
            phase.pressureAtMiddle = findValueAtTime(pressureData, (startTime + endTime) / 2);
            phase.pressureAtEnd = findValueAtTime(pressureData, endTime);

            // Flow metrics
            phase.avgFlow = calculateAverage(flowData, startTime, endTime);
            phase.maxFlow = calculateMax(flowData, startTime, endTime);
            phase.minFlow = calculateMin(flowData, startTime, endTime);
            phase.flowAtStart = findValueAtTime(flowData, startTime);
            phase.flowAtMiddle = findValueAtTime(flowData, (startTime + endTime) / 2);
            phase.flowAtEnd = findValueAtTime(flowData, endTime);

            // Temperature metrics
            phase.avgTemperature = calculateAverage(tempData, startTime, endTime);
            phase.tempStability = calculateStdDev(tempData, startTime, endTime);

            // Weight gained
            double startWeight = findValueAtTime(weightData, startTime);
            double endWeight = findValueAtTime(weightData, endTime);
            phase.weightGained = endWeight - startWeight;

            // Track preinfusion duration
            QString lowerName = phase.name.toLower();
            if (lowerName.contains("preinfus") || lowerName.contains("pre-infus") ||
                lowerName.contains("bloom") || lowerName.contains("soak")) {
                summary.preinfusionDuration += phase.duration;
            } else {
                summary.mainExtractionDuration += phase.duration;
            }

            summary.phases.append(phase);
        }
    }

    // Now detect channeling - find actual extraction phase start time
    // Look for phases named "Extraction", "Pour", "Hold" etc.
    double extractionStartTime = summary.timeToFirstDrip + 3.0;  // Default fallback
    for (const auto& phase : summary.phases) {
        QString lowerName = phase.name.toLower();
        // Skip preparatory phases - only check during actual extraction
        if (lowerName.contains("extract") || lowerName.contains("pour") ||
            lowerName.contains("hold") || lowerName == "main") {
            extractionStartTime = phase.startTime;
            break;
        }
    }
    summary.channelingDetected = detectChanneling(flowData, extractionStartTime);

    return summary;
}

QString ShotSummarizer::buildUserPrompt(const ShotSummary& summary) const
{
    QString prompt;
    QTextStream out(&prompt);

    out << "## Shot Data\n\n";

    out << "**Profile**: " << (summary.profileTitle.isEmpty() ? "Unknown" : summary.profileTitle) << "\n";
    out << "**Dose**: " << QString::number(summary.doseWeight, 'f', 1) << "g";
    out << " → **Yield**: " << QString::number(summary.finalWeight, 'f', 1) << "g";
    out << " (ratio 1:" << QString::number(summary.ratio, 'f', 1) << ")\n";
    out << "**Total Time**: " << QString::number(summary.totalDuration, 'f', 1) << "s\n";
    out << "**Time to first drip**: " << QString::number(summary.timeToFirstDrip, 'f', 1) << "s\n";
    if (summary.preinfusionDuration > 0) {
        out << "**Preinfusion**: " << QString::number(summary.preinfusionDuration, 'f', 1) << "s\n";
    }
    out << "\n";

    // Bean information
    if (!summary.beanBrand.isEmpty() || !summary.beanType.isEmpty() || !summary.roastLevel.isEmpty()) {
        out << "### Coffee\n";
        if (!summary.beanBrand.isEmpty() || !summary.beanType.isEmpty()) {
            out << "- " << summary.beanBrand;
            if (!summary.beanBrand.isEmpty() && !summary.beanType.isEmpty()) out << " - ";
            out << summary.beanType << "\n";
        }
        if (!summary.roastLevel.isEmpty()) out << "- Roast: " << summary.roastLevel << "\n";
        if (!summary.roastDate.isEmpty()) out << "- Roasted: " << summary.roastDate << "\n";
        if (!summary.grinderModel.isEmpty()) {
            out << "- Grinder: " << summary.grinderModel;
            if (!summary.grinderSetting.isEmpty()) out << " @ " << summary.grinderSetting;
            out << "\n";
        }
        out << "\n";
    }

    // Curve data - sampled every 0.5 seconds for AI analysis
    out << "### Curve Data (sampled every 0.5s)\n";
    out << "Format: time | actual pressure (target/limit) | actual flow (target/limit) | actual temp (target) | weight | phase\n";
    out << "CONTROL MODE: If flow target is high (6-8 ml/s), machine controls FLOW and pressure is just a result.\n";
    out << "If pressure target is high (6-11 bar), machine controls PRESSURE and flow is just a result.\n";
    out << "The non-controlled parameter's 'target' is actually a LIMITER (max allowed), not a goal!\n";
    out << "PHASES: The phase column shows ACTUAL PROFILE PHASE NAMES - these are intentional stages, not problems!\n\n";

    double maxTime = summary.totalDuration;

    for (double t = 0; t <= maxTime; t += 0.5) {
        double pressure = findValueAtTime(summary.pressureCurve, t);
        double flow = findValueAtTime(summary.flowCurve, t);
        double temp = findValueAtTime(summary.tempCurve, t);
        double weight = findValueAtTime(summary.weightCurve, t);

        // Get target values (what the profile intended)
        double pressureTarget = findValueAtTime(summary.pressureGoalCurve, t);
        double flowTarget = findValueAtTime(summary.flowGoalCurve, t);
        double tempTarget = findValueAtTime(summary.tempGoalCurve, t);

        // Find the actual phase name at this time from phase markers
        QString phase = "unknown";
        for (const auto& p : summary.phases) {
            if (t >= p.startTime && t < p.endTime) {
                phase = p.name;
                break;
            }
        }
        // Handle last phase (t == endTime)
        if (phase == "unknown" && !summary.phases.isEmpty()) {
            phase = summary.phases.last().name;
        }

        // Format: show actual with target in parentheses if target exists
        out << QString::number(t, 'f', 1) << "s | ";

        // Pressure: show target if available, indicate which mode is active
        if (pressureTarget > 0.1) {
            out << QString::number(pressure, 'f', 1) << " (" << QString::number(pressureTarget, 'f', 1) << ") bar | ";
        } else {
            out << QString::number(pressure, 'f', 1) << " bar | ";
        }

        // Flow: show target if available
        if (flowTarget > 0.1) {
            out << QString::number(flow, 'f', 1) << " (" << QString::number(flowTarget, 'f', 1) << ") ml/s | ";
        } else {
            out << QString::number(flow, 'f', 1) << " ml/s | ";
        }

        // Temperature: always show target (profiles always have temp target)
        if (tempTarget > 0) {
            out << QString::number(temp, 'f', 1) << " (" << QString::number(tempTarget, 'f', 1) << ") °C | ";
        } else {
            out << QString::number(temp, 'f', 1) << "°C | ";
        }

        out << QString::number(weight, 'f', 1) << "g | "
            << phase << "\n";
    }
    out << "\n";

    // Key observations from the curves
    out << "### Curve Analysis\n";

    // Calculate flow stability (standard deviation proxy)
    double flowSum = 0, flowSqSum = 0;
    int flowCount = 0;
    double maxFlow = 0, minFlow = 100;
    for (const auto& phase : summary.phases) {
        if (phase.avgFlow > 0) {
            flowSum += phase.avgFlow;
            flowSqSum += phase.avgFlow * phase.avgFlow;
            flowCount++;
            if (phase.maxFlow > maxFlow) maxFlow = phase.maxFlow;
            if (phase.minFlow < minFlow && phase.minFlow > 0) minFlow = phase.minFlow;
        }
    }
    double avgFlow = flowCount > 0 ? flowSum / flowCount : 0;

    out << "- Average flow during extraction: " << QString::number(avgFlow, 'f', 1) << " ml/s\n";
    out << "- Flow range: " << QString::number(minFlow, 'f', 1) << " - " << QString::number(maxFlow, 'f', 1) << " ml/s\n";

    // Detect concerning patterns (only during main extraction, not preinfusion)
    if (summary.channelingDetected) {
        out << "- ⚠️ CHANNELING DETECTED: Sudden flow spike during main extraction phase\n";
    }
    if (summary.temperatureUnstable) {
        out << "- ⚠️ TEMPERATURE DEVIATION: Actual temp deviates >2°C from target profile\n";
    }
    if (avgFlow > 3.0) {
        out << "- ⚠️ HIGH FLOW: Averaging >" << QString::number(avgFlow, 'f', 1) << " ml/s may indicate low resistance\n";
    }
    if (avgFlow < 1.5 && avgFlow > 0) {
        out << "- ⚠️ LOW FLOW: Averaging <1.5 ml/s may indicate high resistance or choking\n";
    }
    if (summary.totalDuration < 20) {
        out << "- ⚠️ FAST SHOT: " << QString::number(summary.totalDuration, 'f', 0) << "s is shorter than typical\n";
    }
    if (summary.totalDuration > 40) {
        out << "- ⚠️ SLOW SHOT: " << QString::number(summary.totalDuration, 'f', 0) << "s is longer than typical\n";
    }
    out << "\n";

    // Phase breakdown (condensed)
    out << "### Phase Summary\n";
    for (const auto& phase : summary.phases) {
        out << "**" << phase.name << "** (" << QString::number(phase.duration, 'f', 0) << "s): ";
        out << QString::number(phase.avgPressure, 'f', 1) << " bar, ";
        out << QString::number(phase.avgFlow, 'f', 1) << " ml/s, ";
        out << QString::number(phase.avgTemperature, 'f', 0) << "°C";
        if (phase.weightGained > 0) {
            out << ", +" << QString::number(phase.weightGained, 'f', 0) << "g";
        }
        out << "\n";
    }
    out << "\n";

    // User feedback - critical for diagnosis
    out << "### Sensory Feedback\n";
    if (summary.enjoymentScore > 0) {
        out << "- Score: " << summary.enjoymentScore << "/100";
        if (summary.enjoymentScore >= 80) out << " (good)";
        else if (summary.enjoymentScore >= 60) out << " (decent)";
        else if (summary.enjoymentScore >= 40) out << " (needs work)";
        else out << " (problematic)";
        out << "\n";
    }
    if (!summary.tastingNotes.isEmpty()) {
        out << "- Notes: \"" << summary.tastingNotes << "\"\n";
    }
    if (summary.enjoymentScore == 0 && summary.tastingNotes.isEmpty()) {
        out << "- No tasting feedback provided - analyze based on curves only\n";
    }
    out << "\n";

    out << "Analyze the curve data and sensory feedback. Provide ONE specific, evidence-based recommendation.\n";

    return prompt;
}

QString ShotSummarizer::systemPrompt()
{
    return QStringLiteral(R"(You are an expert espresso analyst for the Decent DE1 pressure/flow profiling espresso machine. Your role is to provide precise, evidence-based recommendations by analyzing shot curves and sensory feedback.

## CRITICAL: Avoid Default Advice

DO NOT default to "grind finer" - this is the most overused advice and often wrong. Before suggesting ANY grind change, you MUST cite specific evidence from the curve data. If the flow rate and pressure relationship looks normal for the profile type, grind is likely correct.

## Understanding the DE1

The DE1 is a profiling machine that can control either pressure OR flow (not both simultaneously - they're inversely related through puck resistance). This means:
- In PRESSURE PROFILES: The machine sets pressure, flow is determined by grind/puck
- In FLOW PROFILES: The machine sets flow rate, pressure is determined by grind/puck
- The relationship between set value and resulting value reveals puck resistance

## CRITICAL: Reading Target Values Correctly

The curve data shows BOTH pressure and flow targets, but only ONE is the ACTIVE CONTROL at any time:

### How to identify the control mode:
- **If flow target is high (6-8+ ml/s) and pressure target is low (1-4 bar)**: This is FLOW-CONTROLLED
  - The machine pushes water at the target flow rate
  - Pressure is a RESULT of puck resistance (will be high if grind is fine)
  - High actual pressure with flow target = fine grind/high resistance (NORMAL)
- **If pressure target is high (6-11 bar) and flow target is declining**: This is likely PRESSURE-CONTROLLED
  - The machine maintains target pressure
  - Flow is a RESULT of puck resistance
- **If both targets are shown**: The non-controlling target is usually a LIMITER (max allowed value)
  - e.g., "11 bar" might be the maximum allowed, not what the machine is trying to achieve
  - Look at which actual value is CLOSER to its target - that's the controlled parameter

### Common misinterpretation:
- "Pressure reached 12 bar but target was 3 bar" during a FLOW-CONTROLLED fill phase = NORMAL
  - The machine was pushing 8 ml/s flow, pressure built due to resistance
  - This is the puck resisting flow, not a problem
- "Pressure only reached 4 bar but target was 11 bar" during FLOW-CONTROLLED extraction = NORMAL
  - The 11 bar is a LIMITER, machine is controlling FLOW at 3.5 ml/s
  - Low pressure means low resistance, achieved flow target easily

## CRITICAL: Preinfusion vs Main Extraction

Most profiles have distinct phases - DO NOT confuse preinfusion behavior with extraction problems:

### Preinfusion Phase (typically first 5-15+ seconds)
- **Low pressure is INTENTIONAL** (often 1-4 bar) - this saturates the puck gently
- **Variable/ramping flow is NORMAL** - flow increases as water penetrates the puck
- **Flow ranging from 0.5 to 3+ ml/s during this phase is EXPECTED**
- **This is NOT channeling** - it's the puck absorbing water and flow establishing
- Blooming profiles may have 20-30s+ of intentionally low pressure

### Main Extraction Phase (after preinfusion)
- Flow should STABILIZE here - look for erratic behavior only in THIS phase
- Sudden flow spikes HERE (not during preinfusion) may indicate channeling
- Pressure reaches target profile values

### Common Misdiagnosis to Avoid
- "Flow ranging from 0.8 to 3.4 ml/s" across entire shot = NORMAL (preinfusion ramp + extraction)
- "Low pressure of 2 bar" during first 10s = NORMAL preinfusion, not under-extraction
- "Fast first drip" = Profile design choice, not necessarily channeling
- Long shot times (60-90s) with blooming profiles = INTENTIONAL, not a problem
- Pressure/flow spikes during phase transitions = NORMAL machine behavior
- Declining temperature during shot = Often INTENTIONAL profile design (see below)

## CRITICAL: Understanding Profile Phases

DE1 profiles have NAMED PHASES that execute sequentially. Common phase names include:
- **Prefill/Fill**: Initial water contact, expect pressure/flow ramp-up
- **Preinfusion/Preinfuse**: Low-pressure saturation, variable flow is normal
- **Compressing/Compression**: Puck compression phase, flow drops as puck compacts
- **Dripping**: Waiting for first drops, low flow expected
- **Pressurize/Ramp**: INTENTIONAL pressure increase - spikes here are BY DESIGN
- **Extraction/Pour/Hold**: Main extraction at target pressure/flow
- **Decline/Taper**: Intentional pressure reduction toward end

### Phase Transition Behavior
When the profile transitions between phases, you will see:
- Sudden pressure changes (ramping up or down) - THIS IS INTENTIONAL
- Flow spikes or drops as the machine adjusts - NOT channeling
- These transitions are the profile working correctly

### Declining Temperature Profiles
Many advanced profiles INTENTIONALLY reduce temperature during extraction:
- Start high (92-96°C) for initial extraction
- Decline to lower temp (86-90°C) by end of shot
- This is DESIGNED behavior to balance extraction of different compounds
- A 5-7°C drop over the shot can be INTENTIONAL - check if actual follows target
- Only flag temperature as "unstable" if it deviates significantly FROM THE TARGET

## Reading the Curves - What They Tell You

### Pressure vs Flow Relationship (The Key Diagnostic)
- **High pressure + Low flow** = High resistance (fine grind, dense puck, or restriction)
- **Low pressure + High flow** = Low resistance (coarse grind, channeling, or thin puck)
- **Pressure drops while flow stays constant** = Channeling developing (puck eroding)
- **Flow increases during shot at constant pressure** = Channeling or fines migration
- **Flow decreases during shot** = Fines migration blocking, or puck compression

### Curve Shape Analysis
- **Smooth, predictable curves** = Good puck integrity, proper grind
- **Erratic flow with stable pressure** = Channeling (water finding paths of least resistance)
- **Pressure spikes** = Blockages or air pockets
- **Gradual flow decline** = Normal extraction behavior OR fines clogging
- **Sudden flow changes** = Puck structure failure, channeling onset

### Preinfusion Analysis
- **Long preinfusion, slow pressure build** = Fine grind, good saturation
- **Fast pressure spike in preinfusion** = Too fine, or puck not absorbing
- **No pressure build in preinfusion** = Too coarse, water running through

## Diagnostic Framework (Evidence-Based)

### Signs the GRIND IS CORRECT (do not change):
- Flow rate matches profile expectations (2-2.5 ml/s for most profiles)
- Pressure and flow have smooth, inverse relationship
- Shot time is in expected range (25-35s for standard, varies by profile)
- No erratic flow patterns
- First drips appear at expected time

### Signs to GRIND FINER (requires multiple indicators):
- Flow rate consistently >3 ml/s at target pressure
- Shot finishes too fast (<20s) despite good puck prep
- Thin, watery taste WITH fast flow data
- Pressure can't build to target (in pressure profiles)
- Very short preinfusion before flow starts

### Signs to GRIND COARSER (requires multiple indicators):
- Flow rate consistently <1.5 ml/s at target pressure
- Shot takes >40s despite normal dose
- Bitter/astringent taste WITH evidence of over-extraction in curves
- Pressure overshoots target trying to achieve flow
- Preinfusion takes forever, pressure builds very slowly
- Choking (flow drops to near zero)

### Signs of CHANNELING (not a grind problem):
- Erratic flow at stable pressure
- Sudden flow increases mid-shot
- Pressure drops while flow increases
- Very fast shot but grind was previously dialed in
- FIX: Puck prep, distribution, tamping - NOT grind

### Signs of TEMPERATURE issues:
- Curves look perfect but taste is off
- Sour with good extraction time = temp too low
- Bitter with good extraction time = temp too high
- Temperature instability in data = machine issue

### Signs of DOSE issues:
- Headspace problems (too much = fast flow at edges, too little = puck damage)
- Consistent channeling despite good distribution = dose mismatch for basket
- Flow patterns suggest uneven density

### Signs of RATIO/YIELD issues:
- Good curves, good flow, but taste is unbalanced
- Sourness that appears late in cup = cut the shot earlier
- Bitterness that appears late = cut earlier
- Thin/hollow despite good flow = pull longer

## Profile-Specific Considerations

### Pressure Profiles (Traditional, Lever-style)
- Expect flow to vary based on puck resistance
- Flow should be 1.5-3 ml/s at 6-9 bar for medium roasts
- Declining flow during shot is normal (extraction and compression)

### Flow Profiles (Blooming, Allongé)
- Expect pressure to vary based on resistance
- Pressure should be 4-8 bar at 2-2.5 ml/s for medium roasts
- Rising pressure during shot can indicate fines migration

### Blooming/Saturating Profiles
- Long low-pressure preinfusion (20-40s at 1-3 bar) is INTENTIONAL
- Total shot time of 60-90+ seconds is EXPECTED
- Don't mistake slow start for "too fine"
- Don't mistake flow variations during bloom for "channeling"
- Focus ONLY on main extraction phase (after bloom) for diagnosis
- Low flow during bloom = puck absorbing water (good!)
- Flow increase after bloom = saturation complete, extraction starting

## Roast Level Considerations

### Light Roasts
- NEED higher pressure/temperature, accept longer shots
- Tighter cells = more resistance = looks "too fine" but isn't
- Fruity acidity is good, not under-extraction
- 93-96°C typical, 1:2.5-3 ratio common

### Medium Roasts
- Most forgiving, standard parameters work
- 92-94°C typical, 1:2-2.5 ratio
- Balance of acidity and body expected

### Dark Roasts
- NEED coarser grind, lower temp, shorter ratio
- Very soluble = easy to over-extract
- 88-91°C typical, 1:1.5-2 ratio
- Bitterness can be roast character, not over-extraction

## Response Format

### Analysis
[Describe what the curves show - cite specific data points: "Flow averaged 2.3 ml/s at 8.5 bar, which is within normal range" or "Flow spiked from 1.8 to 3.2 ml/s at 15s while pressure remained stable at 9 bar, indicating channeling"]

### Primary Issue
[Identify ONE main issue with supporting evidence from the data]

### Recommendation
**Adjust**: [Specific parameter]
**How**: [Precise adjustment with reasoning]
**Why**: [Connect the data evidence to this recommendation]

### What to Look For Next Shot
[Specific curve behaviors that will confirm if the adjustment worked]

### Important Context
[Any caveats - e.g., "If this doesn't improve sourness, the issue may be temperature rather than extraction"]

## Rules

1. ALWAYS cite curve data to support recommendations
2. If curves look good, focus on temperature or ratio before grind
3. Never recommend grind changes for channeling - that's puck prep
4. Acknowledge when a shot is already good
5. Consider that "fast" shots can be intentional (turbos, blooming)
6. Consider that "slow" shots can be intentional (lever profiles)
7. Trust tasting notes over data when they conflict - but explain the discrepancy
8. One change at a time, with clear success criteria)");
}

double ShotSummarizer::findValueAtTime(const QVector<QPointF>& data, double time) const
{
    if (data.isEmpty()) return 0;

    // Find closest point
    for (int i = 0; i < data.size(); i++) {
        if (data[i].x() >= time) {
            if (i == 0) return data[i].y();
            // Linear interpolation
            double t = (time - data[i-1].x()) / (data[i].x() - data[i-1].x());
            return data[i-1].y() + t * (data[i].y() - data[i-1].y());
        }
    }
    return data.last().y();
}

double ShotSummarizer::calculateAverage(const QVector<QPointF>& data, double startTime, double endTime) const
{
    if (data.isEmpty()) return 0;

    double sum = 0;
    int count = 0;
    for (const auto& point : data) {
        if (point.x() >= startTime && point.x() <= endTime) {
            sum += point.y();
            count++;
        }
    }
    return count > 0 ? sum / count : 0;
}

double ShotSummarizer::calculateMax(const QVector<QPointF>& data, double startTime, double endTime) const
{
    if (data.isEmpty()) return 0;

    double maxVal = -std::numeric_limits<double>::infinity();
    for (const auto& point : data) {
        if (point.x() >= startTime && point.x() <= endTime) {
            maxVal = std::max(maxVal, point.y());
        }
    }
    return maxVal == -std::numeric_limits<double>::infinity() ? 0 : maxVal;
}

double ShotSummarizer::calculateMin(const QVector<QPointF>& data, double startTime, double endTime) const
{
    if (data.isEmpty()) return 0;

    double minVal = std::numeric_limits<double>::infinity();
    for (const auto& point : data) {
        if (point.x() >= startTime && point.x() <= endTime) {
            minVal = std::min(minVal, point.y());
        }
    }
    return minVal == std::numeric_limits<double>::infinity() ? 0 : minVal;
}

double ShotSummarizer::calculateStdDev(const QVector<QPointF>& data, double startTime, double endTime) const
{
    if (data.isEmpty()) return 0;

    double avg = calculateAverage(data, startTime, endTime);
    double sumSquares = 0;
    int count = 0;

    for (const auto& point : data) {
        if (point.x() >= startTime && point.x() <= endTime) {
            double diff = point.y() - avg;
            sumSquares += diff * diff;
            count++;
        }
    }

    return count > 1 ? std::sqrt(sumSquares / (count - 1)) : 0;
}

double ShotSummarizer::findTimeToFirstDrip(const QVector<QPointF>& flowData) const
{
    const double threshold = 0.5;  // mL/s - when we consider "drip" has started
    for (const auto& point : flowData) {
        if (point.y() >= threshold) {
            return point.x();
        }
    }
    return 0;
}

bool ShotSummarizer::detectChanneling(const QVector<QPointF>& flowData, double afterTime) const
{
    if (flowData.size() < 10) return false;

    // Look for sudden flow spikes (>50% increase in 0.5s) AFTER preinfusion
    // During preinfusion, flow naturally ramps up as water saturates the puck - this is normal
    for (int i = 5; i < flowData.size() - 5; i++) {
        // Skip preinfusion phase - flow variations are expected there
        if (flowData[i].x() < afterTime) continue;

        double prevFlow = flowData[i - 5].y();
        double currFlow = flowData[i].y();

        // Only flag channeling if we see sudden spikes during main extraction
        if (prevFlow > 0.5 && currFlow > prevFlow * 1.5) {
            return true;
        }
    }
    return false;
}
