import QtQuick
import Decenza

// Animated cup-fill visualization for espresso extraction.
// 3D-style cup drawn as an elliptical-rim cylinder with depth shading.
Item {
    id: root

    property real currentWeight: 0
    property real targetWeight: 36
    property real currentFlow: 0
    property real shotTime: 0
    property int phase: 0
    property real currentPressure: 0
    property real goalPressure: 0
    property real goalFlow: 0

    // Tracking color: green = on track, yellow = drifting, red = way off
    // Uses whichever goal is active (pressure or flow control)
    // Don't show tracking during preheat — machine is still ramping up
    readonly property bool hasGoal: (goalPressure > 0 || goalFlow > 0) &&
        phase !== MachineStateType.Phase.EspressoPreheating
    readonly property real trackDelta: {
        if (goalPressure > 0)
            return Math.abs(currentPressure - goalPressure)
        if (goalFlow > 0)
            return Math.abs(currentFlow - goalFlow)
        return 0
    }
    readonly property real trackThresholdGood: goalPressure > 0 ? 0.5 : 0.3
    readonly property real trackThresholdWarn: goalPressure > 0 ? 1.5 : 0.8
    readonly property color trackColor: {
        if (!hasGoal) return Theme.textSecondaryColor
        if (trackDelta < trackThresholdGood) return Theme.trackOnTargetColor
        if (trackDelta < trackThresholdWarn) return Theme.trackDriftingColor
        return Theme.trackOffTargetColor
    }

    // Wave animation phase
    property real wavePhase: 0

    Timer {
        id: animTimer
        interval: 33 // ~30fps
        repeat: true
        running: root.currentFlow > 0.1
        onTriggered: {
            root.wavePhase += 0.15
            canvas.requestPaint()
        }
    }

    onCurrentWeightChanged: canvas.requestPaint()
    onTargetWeightChanged: canvas.requestPaint()
    onCurrentFlowChanged: if (!animTimer.running) canvas.requestPaint()

    Canvas {
        id: canvas
        anchors.fill: parent
        renderStrategy: Canvas.Threaded

        // Helper: draw an ellipse path (for top/bottom rims)
        function ellipsePath(ctx, cx, cy, rx, ry) {
            ctx.beginPath()
            ctx.ellipse(cx - rx, cy - ry, rx * 2, ry * 2)
        }

        // Helper: cup body path (left side down, bottom ellipse arc, right side up)
        function cupBodyPath(ctx, cx, topRx, topRy, topCy, botRx, botRy, botCy) {
            ctx.beginPath()
            // Left side: top-left down to bottom-left
            ctx.moveTo(cx - topRx, topCy)
            ctx.lineTo(cx - botRx, botCy)
            // Bottom arc (front half — from left to right)
            ctx.ellipse(cx - botRx, botCy - botRy, botRx * 2, botRy * 2)
            // We need a custom path: left down, bottom front arc, right up
            // Use bezier to approximate the front arc of the bottom ellipse
        }

        onPaint: {
            var ctx = getContext("2d")
            var w = width
            var h = height
            ctx.reset()
            ctx.clearRect(0, 0, w, h)

            var s = Math.min(w, h)
            var cx = w / 2

            // Cup geometry — slight top-down perspective
            var cupW = s * 0.42          // half-width at top rim
            var cupBotW = cupW * 0.82    // half-width at bottom (tapered)
            var rimOvalH = cupW * 0.22   // ellipse height for rim (perspective)
            var botOvalH = cupBotW * 0.18
            var cupH = s * 0.44          // body height (between rim center and bottom center)
            var rimCy = h * 0.35         // vertical center of top rim (shifted down to clear banners)
            var botCy = rimCy + cupH     // vertical center of bottom

            // Handle
            var handleW = cupW * 0.35
            var handleH = cupH * 0.55
            var handleX = cx + cupW - Theme.scaled(2)
            var handleY = rimCy + cupH * 0.12

            // ---- Saucer (behind cup) — 3D with thickness ----
            var saucerCy = botCy + botOvalH + Theme.scaled(4)
            var saucerRx = cupW * 1.1
            var saucerRy = rimOvalH * 0.55
            var saucerThick = Theme.scaled(8)

            // Saucer side band (gives thickness)
            var saucerSideGrad = ctx.createLinearGradient(cx - saucerRx, 0, cx + saucerRx, 0)
            saucerSideGrad.addColorStop(0, Qt.rgba(0.14, 0.14, 0.17, 1))
            saucerSideGrad.addColorStop(0.4, Qt.rgba(0.28, 0.28, 0.33, 1))
            saucerSideGrad.addColorStop(0.6, Qt.rgba(0.25, 0.25, 0.30, 1))
            saucerSideGrad.addColorStop(1, Qt.rgba(0.12, 0.12, 0.15, 1))

            ctx.beginPath()
            // Top ellipse front arc (left to right)
            for (var si = 0; si <= 20; si++) {
                var sa = Math.PI + (Math.PI * si / 20)
                var sx = cx + saucerRx * Math.cos(sa)
                var sy = saucerCy + saucerRy * Math.sin(sa)
                if (si === 0) ctx.moveTo(sx, sy)
                else ctx.lineTo(sx, sy)
            }
            // Bottom ellipse front arc (right to left)
            for (var sj = 20; sj >= 0; sj--) {
                var sa2 = Math.PI + (Math.PI * sj / 20)
                var sx2 = cx + saucerRx * Math.cos(sa2)
                var sy2 = saucerCy + saucerThick + saucerRy * Math.sin(sa2)
                ctx.lineTo(sx2, sy2)
            }
            ctx.closePath()
            ctx.fillStyle = saucerSideGrad
            ctx.fill()
            ctx.strokeStyle = Qt.rgba(0.5, 0.5, 0.55, 0.4)
            ctx.lineWidth = Theme.scaled(1)
            ctx.stroke()

            // Saucer top surface (ellipse)
            var saucerTopGrad = ctx.createLinearGradient(cx - saucerRx, 0, cx + saucerRx, 0)
            saucerTopGrad.addColorStop(0, Qt.rgba(0.18, 0.18, 0.22, 1))
            saucerTopGrad.addColorStop(0.3, Qt.rgba(0.32, 0.32, 0.38, 1))
            saucerTopGrad.addColorStop(0.55, Qt.rgba(0.35, 0.35, 0.40, 1))
            saucerTopGrad.addColorStop(0.8, Qt.rgba(0.25, 0.25, 0.30, 1))
            saucerTopGrad.addColorStop(1, Qt.rgba(0.15, 0.15, 0.18, 1))

            ctx.beginPath()
            ctx.ellipse(cx - saucerRx, saucerCy - saucerRy, saucerRx * 2, saucerRy * 2)
            ctx.fillStyle = saucerTopGrad
            ctx.fill()
            ctx.strokeStyle = Qt.rgba(0.55, 0.55, 0.6, 0.5)
            ctx.lineWidth = Theme.scaled(1.5)
            ctx.stroke()

            // Saucer highlight arc
            ctx.strokeStyle = Qt.rgba(1, 1, 1, 0.1)
            ctx.lineWidth = Theme.scaled(1.5)
            ctx.beginPath()
            for (var sk = 5; sk <= 15; sk++) {
                var ska = Math.PI + (Math.PI * sk / 20)
                var skx = cx + (saucerRx - Theme.scaled(4)) * Math.cos(ska)
                var sky = saucerCy + (saucerRy - Theme.scaled(2)) * Math.sin(ska)
                if (sk === 5) ctx.moveTo(skx, sky)
                else ctx.lineTo(skx, sky)
            }
            ctx.stroke()

            // ---- Handle (behind cup body) — thick 3D C-shape ----
            var handleThick = Theme.scaled(10)  // visual thickness of handle cross-section

            // Outer curve control points
            var hx0 = handleX
            var hy0 = handleY
            var hx3 = handleX
            var hy3 = handleY + handleH
            var hcx1 = handleX + handleW * 1.4
            var hcy1 = handleY - handleH * 0.05
            var hcx2 = handleX + handleW * 1.4
            var hcy2 = handleY + handleH * 1.05

            // Inner curve (offset inward)
            var ihcx1 = handleX + handleW * 0.75
            var ihcy1 = handleY + handleH * 0.1
            var ihcx2 = handleX + handleW * 0.75
            var ihcy2 = handleY + handleH * 0.9

            // Draw filled handle shape (outer curve forward, inner curve back)
            ctx.beginPath()
            ctx.moveTo(hx0, hy0 - handleThick * 0.3)
            ctx.bezierCurveTo(hcx1, hcy1 - handleThick * 0.3,
                              hcx2, hcy2 + handleThick * 0.3,
                              hx3, hy3 + handleThick * 0.3)
            // Inner return
            ctx.lineTo(hx3, hy3 - handleThick * 0.3)
            ctx.bezierCurveTo(ihcx2, ihcy2,
                              ihcx1, ihcy1,
                              hx0, hy0 + handleThick * 0.3)
            ctx.closePath()

            // Handle gradient (dark edges, lighter center for roundness)
            var handleGrad = ctx.createLinearGradient(handleX, hy0, handleX + handleW * 1.4, hy0)
            handleGrad.addColorStop(0, Qt.rgba(0.20, 0.20, 0.24, 1))
            handleGrad.addColorStop(0.3, Qt.rgba(0.33, 0.33, 0.38, 1))
            handleGrad.addColorStop(0.6, Qt.rgba(0.30, 0.30, 0.35, 1))
            handleGrad.addColorStop(1, Qt.rgba(0.16, 0.16, 0.20, 1))
            ctx.fillStyle = handleGrad
            ctx.fill()
            ctx.strokeStyle = Qt.rgba(0.55, 0.55, 0.6, 0.5)
            ctx.lineWidth = Theme.scaled(1.5)
            ctx.stroke()

            // Handle top highlight
            ctx.strokeStyle = Qt.rgba(1, 1, 1, 0.1)
            ctx.lineWidth = Theme.scaled(1.5)
            ctx.beginPath()
            ctx.moveTo(hx0 + Theme.scaled(4), hy0 - handleThick * 0.15)
            ctx.bezierCurveTo(hcx1 * 0.97 + handleX * 0.03, hcy1 - handleThick * 0.2,
                              hcx2 * 0.5 + hcx1 * 0.5, (hcy1 + hcy2) * 0.5 - handleThick * 0.1,
                              hcx2 * 0.97 + handleX * 0.03, hcy2 * 0.6 + hcy1 * 0.4)
            ctx.stroke()

            // ---- Cup body (cylinder sides + bottom) ----

            // Shadow/gradient on cup body for 3D feel
            var bodyGrad = ctx.createLinearGradient(cx - cupW, 0, cx + cupW, 0)
            bodyGrad.addColorStop(0, Qt.rgba(0.12, 0.12, 0.15, 1))     // dark left edge
            bodyGrad.addColorStop(0.15, Qt.rgba(0.22, 0.22, 0.27, 1))
            bodyGrad.addColorStop(0.45, Qt.rgba(0.30, 0.30, 0.36, 1))  // lighter center-left
            bodyGrad.addColorStop(0.55, Qt.rgba(0.28, 0.28, 0.34, 1))  // center
            bodyGrad.addColorStop(0.85, Qt.rgba(0.18, 0.18, 0.22, 1))
            bodyGrad.addColorStop(1, Qt.rgba(0.10, 0.10, 0.13, 1))     // dark right edge

            // Draw cup body: left wall, bottom front arc, right wall
            ctx.beginPath()
            ctx.moveTo(cx - cupW, rimCy)
            ctx.lineTo(cx - cupBotW, botCy)
            // Bottom front arc (half ellipse, going left to right)
            for (var i = 0; i <= 20; i++) {
                var angle = Math.PI + (Math.PI * i / 20) // PI to 2*PI (bottom half)
                var bx = cx + cupBotW * Math.cos(angle)
                var by = botCy + botOvalH * Math.sin(angle)
                ctx.lineTo(bx, by)
            }
            ctx.lineTo(cx + cupW, rimCy)
            ctx.closePath()
            ctx.fillStyle = bodyGrad
            ctx.fill()

            // ---- Liquid fill (clipped to cup interior) ----
            var fillRatio = root.targetWeight > 0
                ? Math.min(root.currentWeight / root.targetWeight, 1.0) : 0
            var interiorH = cupH - Theme.scaled(4)
            var fillH = fillRatio * interiorH
            var fillTopY = botCy - fillH

            if (fillH > 1) {
                ctx.save()

                // Clip to cup interior (walls + front bottom arc only)
                var clipInset = Theme.scaled(5)
                var clipBotRx = cupBotW - clipInset
                var clipBotRy = botOvalH - Theme.scaled(4)
                ctx.beginPath()
                // Start top-left, go down left wall
                ctx.moveTo(cx - cupW + clipInset, rimCy)
                ctx.lineTo(cx - clipBotRx, botCy)
                // Front bottom arc (left to right, PI to 2*PI)
                for (var ci = 0; ci <= 20; ci++) {
                    var ca = Math.PI + (Math.PI * ci / 20)
                    ctx.lineTo(cx + clipBotRx * Math.cos(ca),
                               botCy + clipBotRy * Math.sin(ca))
                }
                // Up right wall
                ctx.lineTo(cx + cupW - clipInset, rimCy)
                // Close across the top
                ctx.closePath()
                ctx.clip()

                // Liquid body gradient
                var liqGrad = ctx.createLinearGradient(cx - cupW, 0, cx + cupW, 0)
                liqGrad.addColorStop(0, "#2a1200")
                liqGrad.addColorStop(0.3, "#4a2410")
                liqGrad.addColorStop(0.5, "#6B3A1F")
                liqGrad.addColorStop(0.7, "#4a2410")
                liqGrad.addColorStop(1, "#2a1200")

                // Liquid top gradient (lighter toward surface)
                var liqVertGrad = ctx.createLinearGradient(0, botCy + botOvalH, 0, fillTopY)
                liqVertGrad.addColorStop(0, "#3E1A00")
                liqVertGrad.addColorStop(0.5, "#6B3A1F")
                liqVertGrad.addColorStop(0.85, "#c49a6c")
                liqVertGrad.addColorStop(1.0, "#d4a760")

                // Draw liquid with wave surface
                var waveAmp = root.currentFlow > 0.1 ? Theme.scaled(2.5) : 0
                var wSteps = 24

                // Interpolate width at fill level
                var t = fillH / interiorH
                var fillRx = cupBotW + (cupW - cupBotW) * t
                var fillOvalH = botOvalH + (rimOvalH - botOvalH) * t

                ctx.beginPath()
                // Start at bottom-left, trace the front bottom arc, up to fill level
                ctx.moveTo(cx - clipBotRx, botCy)
                for (var bfi = 0; bfi <= 20; bfi++) {
                    var bfa = Math.PI + (Math.PI * bfi / 20)
                    ctx.lineTo(cx + clipBotRx * Math.cos(bfa),
                               botCy + clipBotRy * Math.sin(bfa))
                }
                // Up right side to fill level
                ctx.lineTo(cx + fillRx, fillTopY)

                // Wave surface (right to left)
                for (var wi = wSteps; wi >= 0; wi--) {
                    var wFrac = wi / wSteps
                    var wx = cx - fillRx + fillRx * 2 * wFrac
                    var wy = fillTopY + Math.sin(root.wavePhase + wi * 0.5) * waveAmp
                    ctx.lineTo(wx, wy)
                }
                ctx.closePath()
                ctx.fillStyle = liqVertGrad
                ctx.fill()

                // Left-right shading on liquid (layered on top)
                ctx.globalAlpha = 0.4
                ctx.fillStyle = liqGrad
                ctx.fill()
                ctx.globalAlpha = 1.0

                // Crema ellipse at liquid surface
                if (fillRatio > 0.2) {
                    var cremaAlpha = Math.min((fillRatio - 0.2) / 0.3, 0.5)
                    ctx.strokeStyle = Qt.rgba(1, 0.92, 0.75, cremaAlpha)
                    ctx.lineWidth = Theme.scaled(3)
                    ctx.beginPath()
                    for (var cj = 0; cj <= wSteps; cj++) {
                        var cf = cj / wSteps
                        var crx = cx - fillRx * 0.85 + fillRx * 1.7 * cf
                        var cry = fillTopY + Math.sin(root.wavePhase + cj * 0.5) * waveAmp
                        if (cj === 0) ctx.moveTo(crx, cry)
                        else ctx.lineTo(crx, cry)
                    }
                    ctx.stroke()

                    // Subtle inner crema ring
                    ctx.strokeStyle = Qt.rgba(1, 0.88, 0.65, cremaAlpha * 0.4)
                    ctx.lineWidth = Theme.scaled(5)
                    ctx.beginPath()
                    for (var ck = 0; ck <= wSteps; ck++) {
                        var ckf = ck / wSteps
                        var ckx = cx - fillRx * 0.6 + fillRx * 1.2 * ckf
                        var cky = fillTopY + Theme.scaled(3) + Math.sin(root.wavePhase + ck * 0.5) * waveAmp * 0.5
                        if (ck === 0) ctx.moveTo(ckx, cky)
                        else ctx.lineTo(ckx, cky)
                    }
                    ctx.stroke()
                }

                ctx.restore()
            }

            // ---- Pour stream (only during pouring/preinfusion, not preheat) ----
            var isPouring = root.phase === MachineStateType.Phase.Preinfusion ||
                            root.phase === MachineStateType.Phase.Pouring
            if (isPouring && root.currentFlow > 0.3 && fillRatio < 1.0) {
                var streamW = Theme.scaled(2) + root.currentFlow * Theme.scaled(1.2)
                streamW = Math.min(streamW, Theme.scaled(7))
                var streamX = cx
                var streamTop = rimCy - rimOvalH - Theme.scaled(30)
                // Stream ends at liquid surface, or mid-cup if nearly empty
                var streamMaxBot = rimCy + cupH * 0.4
                var streamBot = fillH > 1 ? Math.min(fillTopY, streamMaxBot) : streamMaxBot

                // Stream tapers as it falls
                var streamGrad = ctx.createLinearGradient(0, streamTop, 0, streamBot)
                streamGrad.addColorStop(0, Qt.rgba(0.65, 0.45, 0.25, 0.3))
                streamGrad.addColorStop(0.5, Qt.rgba(0.55, 0.35, 0.18, 0.6))
                streamGrad.addColorStop(1, Qt.rgba(0.5, 0.3, 0.15, 0.8))

                ctx.beginPath()
                ctx.moveTo(streamX - streamW * 0.4, streamTop)
                ctx.lineTo(streamX + streamW * 0.4, streamTop)
                ctx.lineTo(streamX + streamW * 0.6, streamBot)
                ctx.lineTo(streamX - streamW * 0.6, streamBot)
                ctx.closePath()
                ctx.fillStyle = streamGrad
                ctx.fill()
            }

            // ---- Cup outline (walls) — colored by tracking status ----
            var outlineColor = root.hasGoal ? root.trackColor : Theme.textSecondaryColor
            ctx.strokeStyle = outlineColor
            ctx.lineWidth = Theme.scaled(2.5)
            ctx.lineCap = "round"

            // Left wall
            ctx.beginPath()
            ctx.moveTo(cx - cupW, rimCy)
            ctx.lineTo(cx - cupBotW, botCy)
            ctx.stroke()

            // Right wall
            ctx.beginPath()
            ctx.moveTo(cx + cupW, rimCy)
            ctx.lineTo(cx + cupBotW, botCy)
            ctx.stroke()

            // Bottom ellipse (front half only — back is hidden)
            ctx.beginPath()
            for (var bi = 0; bi <= 20; bi++) {
                var ba = Math.PI + (Math.PI * bi / 20)
                var bbx = cx + cupBotW * Math.cos(ba)
                var bby = botCy + botOvalH * Math.sin(ba)
                if (bi === 0) ctx.moveTo(bbx, bby)
                else ctx.lineTo(bbx, bby)
            }
            ctx.stroke()

            // ---- Top rim (ellipse) — the 3D key ----
            // Back rim (behind liquid)
            ctx.strokeStyle = Qt.rgba(0.4, 0.4, 0.45, 0.4)
            ctx.lineWidth = Theme.scaled(2)
            ctx.beginPath()
            for (var ri = 0; ri <= 20; ri++) {
                var ra = Math.PI * ri / 20  // 0 to PI (back half)
                var rrx = cx + cupW * Math.cos(ra)
                var rry = rimCy + rimOvalH * Math.sin(ra)
                if (ri === 0) ctx.moveTo(rrx, rry)
                else ctx.lineTo(rrx, rry)
            }
            ctx.stroke()

            // Front rim (brighter, in front)
            ctx.strokeStyle = outlineColor
            ctx.lineWidth = Theme.scaled(3)
            ctx.beginPath()
            for (var fi = 0; fi <= 20; fi++) {
                var fa = Math.PI + (Math.PI * fi / 20)  // PI to 2*PI (front half)
                var frx = cx + cupW * Math.cos(fa)
                var fry = rimCy + rimOvalH * Math.sin(fa)
                if (fi === 0) ctx.moveTo(frx, fry)
                else ctx.lineTo(frx, fry)
            }
            ctx.stroke()

            // Rim highlight (specular on top edge)
            ctx.strokeStyle = Qt.rgba(1, 1, 1, 0.12)
            ctx.lineWidth = Theme.scaled(1.5)
            ctx.beginPath()
            for (var hi = 4; hi <= 16; hi++) {
                var ha = Math.PI + (Math.PI * hi / 20)
                var hx = cx + (cupW - Theme.scaled(1)) * Math.cos(ha)
                var hy = rimCy + (rimOvalH - Theme.scaled(1.5)) * Math.sin(ha)
                if (hi === 4) ctx.moveTo(hx, hy)
                else ctx.lineTo(hx, hy)
            }
            ctx.stroke()

            // ---- Body highlight (left specular strip) ----
            ctx.strokeStyle = Qt.rgba(1, 1, 1, 0.07)
            ctx.lineWidth = Theme.scaled(6)
            ctx.beginPath()
            ctx.moveTo(cx - cupW * 0.72, rimCy + cupH * 0.08)
            ctx.lineTo(cx - cupBotW * 0.72, botCy - cupH * 0.05)
            ctx.stroke()

            // ---- Completion glow ----
            if (root.currentWeight >= root.targetWeight && root.targetWeight > 0) {
                ctx.shadowColor = Theme.successColor
                ctx.shadowBlur = Theme.scaled(20)
                ctx.strokeStyle = Theme.successColor
                ctx.lineWidth = Theme.scaled(1.5)
                // Glow on front rim
                ctx.beginPath()
                for (var gi = 0; gi <= 20; gi++) {
                    var ga = Math.PI + (Math.PI * gi / 20)
                    var gx = cx + cupW * Math.cos(ga)
                    var gy = rimCy + rimOvalH * Math.sin(ga)
                    if (gi === 0) ctx.moveTo(gx, gy)
                    else ctx.lineTo(gx, gy)
                }
                ctx.stroke()
                ctx.shadowBlur = 0
            }
        }
    }

    // Weight text overlay
    Column {
        anchors.centerIn: parent
        anchors.verticalCenterOffset: parent.height * 0.12
        spacing: Theme.scaled(2)

        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            text: root.currentWeight.toFixed(1) + "g"
            color: Theme.textColor
            font.pixelSize: Theme.scaled(38)
            font.weight: Font.Bold
        }

        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            text: root.targetWeight > 0
                ? TranslationManager.translate("espresso.cupFill.target", "target") + " " + root.targetWeight.toFixed(0) + "g"
                : ""
            color: Theme.textSecondaryColor
            font.pixelSize: Theme.scaled(16)
        }
    }

}
