import QtQuick

Item {
    id: root

    property int pipeCount: 3
    property real speed: 1.0
    property bool running: true

    // 3D scene parameters
    readonly property real fov: 400  // Field of view for perspective
    readonly property real cameraZ: -500
    readonly property vector3d sceneCenter: Qt.vector3d(width / 2, height / 2, 300)

    // Pipe colors - classic Windows screensaver palette
    readonly property var pipeColors: [
        "#C0C0C0",  // Silver
        "#FFD700",  // Gold
        "#CD7F32",  // Bronze/Copper
        "#4169E1",  // Royal Blue
        "#DC143C",  // Crimson
        "#228B22",  // Forest Green
        "#9932CC",  // Dark Orchid
        "#FF6347"   // Tomato
    ]

    // Active pipes data
    property var pipes: []
    property int segmentCounter: 0

    Component.onCompleted: {
        initializePipes()
    }

    function initializePipes() {
        pipes = []
        for (var i = 0; i < pipeCount; i++) {
            pipes.push(createNewPipe(i))
        }
        segmentCounter = 0
    }

    function createNewPipe(index) {
        var colorIndex = index % pipeColors.length
        return {
            segments: [],
            currentPos: Qt.vector3d(
                Math.random() * width * 0.6 + width * 0.2,
                Math.random() * height * 0.6 + height * 0.2,
                Math.random() * 400 + 100
            ),
            direction: randomDirection(),
            color: pipeColors[colorIndex],
            pipeRadius: 12 + Math.random() * 8,
            segmentLength: 40 + Math.random() * 30,
            turnsUntilChange: Math.floor(Math.random() * 5) + 2,
            moveCounter: 0,
            active: true
        }
    }

    function randomDirection() {
        var dirs = [
            Qt.vector3d(1, 0, 0), Qt.vector3d(-1, 0, 0),
            Qt.vector3d(0, 1, 0), Qt.vector3d(0, -1, 0),
            Qt.vector3d(0, 0, 1), Qt.vector3d(0, 0, -1)
        ]
        return dirs[Math.floor(Math.random() * dirs.length)]
    }

    function getPerpendicularDirection(current) {
        var options = []
        if (Math.abs(current.x) > 0.5) {
            options = [
                Qt.vector3d(0, 1, 0), Qt.vector3d(0, -1, 0),
                Qt.vector3d(0, 0, 1), Qt.vector3d(0, 0, -1)
            ]
        } else if (Math.abs(current.y) > 0.5) {
            options = [
                Qt.vector3d(1, 0, 0), Qt.vector3d(-1, 0, 0),
                Qt.vector3d(0, 0, 1), Qt.vector3d(0, 0, -1)
            ]
        } else {
            options = [
                Qt.vector3d(1, 0, 0), Qt.vector3d(-1, 0, 0),
                Qt.vector3d(0, 1, 0), Qt.vector3d(0, -1, 0)
            ]
        }
        return options[Math.floor(Math.random() * options.length)]
    }

    function project3Dto2D(pos3d) {
        var z = pos3d.z - cameraZ
        if (z <= 0) z = 1
        var scale = fov / z
        return {
            x: sceneCenter.x + (pos3d.x - sceneCenter.x) * scale,
            y: sceneCenter.y + (pos3d.y - sceneCenter.y) * scale,
            scale: scale,
            z: pos3d.z
        }
    }

    function isOutOfBounds(pos) {
        var margin = 100
        return pos.x < -margin || pos.x > width + margin ||
               pos.y < -margin || pos.y > height + margin ||
               pos.z < -100 || pos.z > 800
    }

    Timer {
        id: animationTimer
        interval: 50
        running: root.running && root.visible
        repeat: true
        onTriggered: {
            updatePipes()
            canvas.requestPaint()
        }
    }

    function updatePipes() {
        for (var i = 0; i < pipes.length; i++) {
            var pipe = pipes[i]
            if (!pipe.active) continue

            pipe.moveCounter++

            // Add segment at regular intervals
            if (pipe.moveCounter >= 3) {
                pipe.moveCounter = 0

                var startPos = pipe.currentPos
                var endPos = Qt.vector3d(
                    startPos.x + pipe.direction.x * pipe.segmentLength,
                    startPos.y + pipe.direction.y * pipe.segmentLength,
                    startPos.z + pipe.direction.z * pipe.segmentLength
                )

                // Add segment with joint info
                var hasJoint = pipe.turnsUntilChange <= 0
                pipe.segments.push({
                    start: startPos,
                    end: endPos,
                    hasJoint: hasJoint,
                    radius: pipe.pipeRadius
                })

                pipe.currentPos = endPos
                pipe.turnsUntilChange--

                // Change direction
                if (pipe.turnsUntilChange <= 0) {
                    pipe.direction = getPerpendicularDirection(pipe.direction)
                    pipe.turnsUntilChange = Math.floor(Math.random() * 6) + 2
                }

                // Check bounds and reset if needed
                if (isOutOfBounds(endPos)) {
                    // Reset this pipe
                    pipes[i] = createNewPipe(i)
                }

                // Limit segment count per pipe
                if (pipe.segments.length > 100) {
                    pipe.segments.shift()
                }
            }
        }

        segmentCounter++

        // Periodically reset all pipes to prevent infinite growth
        if (segmentCounter > 500) {
            initializePipes()
        }
    }

    Canvas {
        id: canvas
        anchors.fill: parent

        onPaint: {
            var ctx = getContext("2d")
            ctx.reset()

            // Dark background with slight gradient
            var bgGrad = ctx.createLinearGradient(0, 0, 0, height)
            bgGrad.addColorStop(0, "#0a0a1a")
            bgGrad.addColorStop(1, "#000008")
            ctx.fillStyle = bgGrad
            ctx.fillRect(0, 0, width, height)

            // Collect all segments with their depth for sorting
            var allSegments = []

            for (var i = 0; i < pipes.length; i++) {
                var pipe = pipes[i]
                for (var j = 0; j < pipe.segments.length; j++) {
                    var seg = pipe.segments[j]
                    var avgZ = (seg.start.z + seg.end.z) / 2
                    allSegments.push({
                        segment: seg,
                        color: pipe.color,
                        depth: avgZ
                    })
                }
            }

            // Sort by depth (back to front)
            allSegments.sort(function(a, b) { return a.depth - b.depth })

            // Draw segments
            for (var k = 0; k < allSegments.length; k++) {
                drawPipeSegment(ctx, allSegments[k].segment, allSegments[k].color)
            }
        }

        function drawPipeSegment(ctx, seg, color) {
            var start2d = project3Dto2D(seg.start)
            var end2d = project3Dto2D(seg.end)

            var avgScale = (start2d.scale + end2d.scale) / 2
            var lineWidth = seg.radius * avgScale * 2

            if (lineWidth < 1) return  // Skip very small segments

            // Calculate pipe gradient for 3D effect
            var dx = end2d.x - start2d.x
            var dy = end2d.y - start2d.y
            var len = Math.sqrt(dx * dx + dy * dy)

            if (len < 0.5) {
                // Draw joint ball
                drawJoint(ctx, start2d, seg.radius * avgScale, color)
                return
            }

            // Perpendicular direction for gradient
            var perpX = -dy / len
            var perpY = dx / len
            var gradOffset = lineWidth / 2

            var midX = (start2d.x + end2d.x) / 2
            var midY = (start2d.y + end2d.y) / 2

            // Create gradient across the pipe for 3D cylinder look
            var grad = ctx.createLinearGradient(
                midX - perpX * gradOffset, midY - perpY * gradOffset,
                midX + perpX * gradOffset, midY + perpY * gradOffset
            )

            var baseColor = Qt.darker(color, 1.5)
            var highlightColor = Qt.lighter(color, 1.4)
            var midColor = color

            grad.addColorStop(0, baseColor.toString())
            grad.addColorStop(0.3, highlightColor.toString())
            grad.addColorStop(0.5, midColor.toString())
            grad.addColorStop(0.7, midColor.toString())
            grad.addColorStop(1, baseColor.toString())

            ctx.strokeStyle = grad
            ctx.lineWidth = lineWidth
            ctx.lineCap = "round"
            ctx.lineJoin = "round"

            ctx.beginPath()
            ctx.moveTo(start2d.x, start2d.y)
            ctx.lineTo(end2d.x, end2d.y)
            ctx.stroke()

            // Draw joint if this segment has one
            if (seg.hasJoint) {
                drawJoint(ctx, end2d, seg.radius * avgScale * 1.3, color)
            }
        }

        function drawJoint(ctx, pos, radius, color) {
            if (radius < 2) return

            // Draw a spherical joint with 3D shading
            var grad = ctx.createRadialGradient(
                pos.x - radius * 0.3, pos.y - radius * 0.3, 0,
                pos.x, pos.y, radius
            )

            var highlightColor = Qt.lighter(color, 1.5)
            var baseColor = Qt.darker(color, 1.3)

            grad.addColorStop(0, highlightColor.toString())
            grad.addColorStop(0.5, color.toString())
            grad.addColorStop(1, baseColor.toString())

            ctx.fillStyle = grad
            ctx.beginPath()
            ctx.arc(pos.x, pos.y, radius, 0, Math.PI * 2)
            ctx.fill()
        }
    }

    // Reset button (hidden, for programmatic reset)
    function reset() {
        initializePipes()
        canvas.requestPaint()
    }

    // Adjust pipe count
    function setPipeCount(count) {
        pipeCount = Math.max(1, Math.min(10, count))
        initializePipes()
    }
}
