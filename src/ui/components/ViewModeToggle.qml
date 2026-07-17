import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

RowLayout {
    id: root
    spacing: 5

    property var modes: ["List", "Grid"]
    property string currentMode: "List"
    signal modeSelected(string mode)

    focus: true

    Keys.onPressed: (event) => {
        if (event.key === Qt.Key_Enter || event.key === Qt.Key_Return || event.key === Qt.Key_Space) {
            var currentIndex = root.modes.indexOf(root.currentMode);
            var nextIndex = (currentIndex + 1) % root.modes.length;
            root.modeSelected(root.modes[nextIndex]);
            event.accepted = true;
        }
    }

    Repeater {
        model: root.modes
        Button {
            text: modelData
            background: Rectangle {
                color: modelData === root.currentMode ? "#FFD54F" : "#1E1E1E"
                radius: 4
                implicitHeight: 30
                implicitWidth: 60
                border.color: root.activeFocus ? "#FFFFFF" : "transparent"
                border.width: root.activeFocus && modelData === root.currentMode ? 2 : 0
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
