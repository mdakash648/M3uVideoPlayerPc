import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

RowLayout {
    id: root
    spacing: 5

    property var modes: ["List", "Grid"]
    property string currentMode: "List"
    signal modeSelected(string mode)

    Repeater {
        model: root.modes
        Button {
            text: modelData
            background: Rectangle {
                color: modelData === root.currentMode ? "#FFD54F" : "#1E1E1E"
                radius: 4
                implicitHeight: 30
                implicitWidth: 60
            }
            contentItem: Text {
                text: modelData
                color: modelData === root.currentMode ? "#121212" : "#FFFFFF"
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                font.pixelSize: 12
            }
            onClicked: root.modeSelected(modelData)
        }
    }
}
