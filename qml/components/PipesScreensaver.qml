import QtQuick
import QtQuick3D
import QtQuick3D.Helpers

Item {
    id: root

    property int pipeCount: 4
    property real speed: 1.0
    property bool running: true

    // Pipe colors - metallic materials
    readonly property var pipeColors: [
        "#C0C0C0",  // Silver
        "#FFD700",  // Gold
        "#CD7F32",  // Bronze
        "#4169E1",  // Royal Blue
        "#DC143C",  // Crimson
        "#228B22",  // Forest Green
        "#9932CC",  // Dark Orchid
        "#00CED1"   // Dark Turquoise
    ]

    // Scene bounds
    readonly property real sceneSize: 400
    readonly property real pipeRadius: 8
    readonly property real jointRadius: 12
    readonly property real segmentLength: 60

    // Pipe data storage
    property var pipes: []
    property var segmentNodes: []

    Component.onCompleted: {
        initializePipes()
    }

    function initializePipes() {
        // Clear existing segments
        for (var i = 0; i < segmentNodes.length; i++) {
            segmentNodes[i].destroy()
        }
        segmentNodes = []
        pipes = []

        for (var p = 0; p < pipeCount; p++) {
            pipes.push({
                pos: Qt.vector3d(
                    (Math.random() - 0.5) * sceneSize * 0.6,
                    (Math.random() - 0.5) * sceneSize * 0.6,
                    (Math.random() - 0.5) * sceneSize * 0.6
                ),
                dir: randomDirection(),
                colorIndex: p % pipeColors.length,
                turnsLeft: Math.floor(Math.random() * 4) + 2,
                segmentCount: 0
            })
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

    function perpendicularDirection(current) {
        var options = []
        if (Math.abs(current.x) > 0.5) {
            options = [Qt.vector3d(0, 1, 0), Qt.vector3d(0, -1, 0),
                       Qt.vector3d(0, 0, 1), Qt.vector3d(0, 0, -1)]
        } else if (Math.abs(current.y) > 0.5) {
            options = [Qt.vector3d(1, 0, 0), Qt.vector3d(-1, 0, 0),
                       Qt.vector3d(0, 0, 1), Qt.vector3d(0, 0, -1)]
        } else {
            options = [Qt.vector3d(1, 0, 0), Qt.vector3d(-1, 0, 0),
                       Qt.vector3d(0, 1, 0), Qt.vector3d(0, -1, 0)]
        }
        return options[Math.floor(Math.random() * options.length)]
    }

    function isOutOfBounds(pos) {
        var limit = sceneSize * 0.45
        return Math.abs(pos.x) > limit || Math.abs(pos.y) > limit || Math.abs(pos.z) > limit
    }

    function directionToRotation(dir) {
        // Cylinder default orientation is along Y axis
        // We need to rotate it to align with the direction
        if (Math.abs(dir.y) > 0.5) {
            // Already along Y
            return Qt.vector3d(0, 0, 0)
        } else if (Math.abs(dir.x) > 0.5) {
            // Along X - rotate 90 degrees around Z
            return Qt.vector3d(0, 0, 90)
        } else {
            // Along Z - rotate 90 degrees around X
            return Qt.vector3d(90, 0, 0)
        }
    }

    function addPipeSegment(pipeIndex) {
        var pipe = pipes[pipeIndex]
        var startPos = pipe.pos
        var endPos = Qt.vector3d(
            startPos.x + pipe.dir.x * segmentLength,
            startPos.y + pipe.dir.y * segmentLength,
            startPos.z + pipe.dir.z * segmentLength
        )

        // Calculate midpoint for cylinder position
        var midPoint = Qt.vector3d(
            (startPos.x + endPos.x) / 2,
            (startPos.y + endPos.y) / 2,
            (startPos.z + endPos.z) / 2
        )

        var rotation = directionToRotation(pipe.dir)
        var color = pipeColors[pipe.colorIndex]

        // Create cylinder segment
        var cylinder = cylinderComponent.createObject(sceneRoot, {
            "position": midPoint,
            "eulerRotation": rotation,
            "pipeColor": color,
            "segmentLength": segmentLength
        })
        segmentNodes.push(cylinder)

        // Create joint sphere at the end
        var joint = jointComponent.createObject(sceneRoot, {
            "position": endPos,
            "pipeColor": color
        })
        segmentNodes.push(joint)

        // Update pipe state
        pipe.pos = endPos
        pipe.turnsLeft--
        pipe.segmentCount++

        // Change direction if needed
        if (pipe.turnsLeft <= 0) {
            pipe.dir = perpendicularDirection(pipe.dir)
            pipe.turnsLeft = Math.floor(Math.random() * 5) + 2
        }

        // Reset pipe if out of bounds or too long
        if (isOutOfBounds(endPos) || pipe.segmentCount > 25) {
            pipe.pos = Qt.vector3d(
                (Math.random() - 0.5) * sceneSize * 0.5,
                (Math.random() - 0.5) * sceneSize * 0.5,
                (Math.random() - 0.5) * sceneSize * 0.5
            )
            pipe.dir = randomDirection()
            pipe.colorIndex = (pipe.colorIndex + 1) % pipeColors.length
            pipe.turnsLeft = Math.floor(Math.random() * 4) + 2
            pipe.segmentCount = 0
        }

        // Limit total segments to prevent memory growth
        while (segmentNodes.length > 300) {
            var old = segmentNodes.shift()
            old.destroy()
        }
    }

    Timer {
        id: growTimer
        interval: 80 / speed
        running: root.running && root.visible
        repeat: true
        onTriggered: {
            for (var i = 0; i < pipes.length; i++) {
                addPipeSegment(i)
            }
        }
    }

    // Slow camera orbit
    property real cameraAngle: 0
    NumberAnimation on cameraAngle {
        from: 0
        to: 360
        duration: 60000
        loops: Animation.Infinite
        running: root.running && root.visible
    }

    View3D {
        id: view3d
        anchors.fill: parent
        environment: sceneEnvironment

        SceneEnvironment {
            id: sceneEnvironment
            clearColor: "#050510"
            backgroundMode: SceneEnvironment.Color
            antialiasingMode: SceneEnvironment.MSAA
            antialiasingQuality: SceneEnvironment.High
            temporalAAEnabled: true
            temporalAAStrength: 1.0
        }

        // Orbiting camera
        PerspectiveCamera {
            id: camera
            position: Qt.vector3d(
                Math.cos(cameraAngle * Math.PI / 180) * 600,
                150 + Math.sin(cameraAngle * 0.5 * Math.PI / 180) * 100,
                Math.sin(cameraAngle * Math.PI / 180) * 600
            )
            eulerRotation.y: -cameraAngle
            eulerRotation.x: -10
            clipNear: 10
            clipFar: 2000
            fieldOfView: 50
        }

        // Lighting
        DirectionalLight {
            eulerRotation.x: -45
            eulerRotation.y: -45
            brightness: 1.0
            ambientColor: Qt.rgba(0.2, 0.2, 0.25, 1.0)
            color: "white"
            castsShadow: true
            shadowMapQuality: Light.ShadowMapQualityMedium
        }

        DirectionalLight {
            eulerRotation.x: 30
            eulerRotation.y: 135
            brightness: 0.4
            color: "#aaccff"
        }

        PointLight {
            position: Qt.vector3d(0, 200, 0)
            brightness: 0.3
            color: "#ffffee"
        }

        // Root node for all pipe segments
        Node {
            id: sceneRoot
        }
    }

    // Cylinder segment component
    Component {
        id: cylinderComponent

        Model {
            id: cylinderModel
            property color pipeColor: "silver"
            property real segmentLength: 60

            source: "#Cylinder"
            scale: Qt.vector3d(pipeRadius / 50, segmentLength / 100, pipeRadius / 50)

            materials: [
                PrincipledMaterial {
                    baseColor: cylinderModel.pipeColor
                    metalness: 0.9
                    roughness: 0.25
                    specularAmount: 1.0
                }
            ]

            // Fade in animation
            opacity: 0
            NumberAnimation on opacity {
                from: 0
                to: 1
                duration: 200
                running: true
            }
        }
    }

    // Joint sphere component
    Component {
        id: jointComponent

        Model {
            id: jointModel
            property color pipeColor: "silver"

            source: "#Sphere"
            scale: Qt.vector3d(jointRadius / 50, jointRadius / 50, jointRadius / 50)

            materials: [
                PrincipledMaterial {
                    baseColor: jointModel.pipeColor
                    metalness: 0.9
                    roughness: 0.2
                    specularAmount: 1.0
                }
            ]

            opacity: 0
            NumberAnimation on opacity {
                from: 0
                to: 1
                duration: 200
                running: true
            }
        }
    }

    function reset() {
        initializePipes()
    }

    function setPipeCount(count) {
        pipeCount = Math.max(1, Math.min(8, count))
        initializePipes()
    }
}
