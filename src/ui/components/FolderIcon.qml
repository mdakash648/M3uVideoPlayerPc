import QtQuick

Item {
    id: root
    property int size: 40

    implicitWidth: size
    implicitHeight: size * 0.78

    // Back panel (slightly darker, peeking out behind the front flap)
    Rectangle {
        id: back
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        height: parent.height * 0.86
        radius: root.size * 0.06

        gradient: Gradient {
            orientation: Gradient.Vertical
            GradientStop { position: 0.0; color: "#FFCA3A" }
            GradientStop { position: 1.0; color: "#F5A623" }
        }
    }

    // Tab sticking out of the top-left of the folder
    Rectangle {
        id: tab
        x: 0
        y: 0
        width: parent.width * 0.42
        height: parent.height * 0.24
        radius: root.size * 0.05
        color: "#FFCA3A"
    }

    // Squares off the bottom-right corner of the tab so it blends into the body
    Rectangle {
        anchors.left: tab.left
        anchors.bottom: tab.bottom
        width: tab.width
        height: tab.height / 2
        color: "#FFCA3A"
    }

    // Front flap (brighter, sits on top, with the classic diagonal-cut top-left corner)
    Rectangle {
        id: front
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 0
        y: parent.height * 0.30
        height: parent.height * 0.70
        radius: root.size * 0.07

        gradient: Gradient {
            orientation: Gradient.Vertical
            GradientStop { position: 0.0; color: "#FFDE6B" }
            GradientStop { position: 1.0; color: "#FFC845" }
        }

        border.color: "#E0971A"
        border.width: Math.max(1, root.size * 0.02)
    }

    // Subtle top highlight on the front flap for a glossy look
    Rectangle {
        anchors.left: front.left
        anchors.right: front.right
        anchors.top: front.top
        anchors.leftMargin: front.radius
        anchors.rightMargin: front.radius
        height: 2
        color: "#FFFFFF"
        opacity: 0.35
    }
}
