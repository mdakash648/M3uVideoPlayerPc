import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// "Continue watching?" dialog for movies/series with a saved resume point.
// Same visual language as the delete confirmation popup in ChannelListView.
Popup {
    id: root
    anchors.centerIn: parent
    width: 420
    modal: true
    focus: true
    padding: 20
    background: Rectangle { color: "#1E1E1E"; radius: 12; border.color: "#333333"; border.width: 1 }

    // Payload passed through to whichever choice the user makes
    property var payload: null
    property real positionMs: 0
    property real durationMs: 0

    signal continueChosen(var payload)
    signal startOverChosen(var payload)

    function formatTime(ms) {
        var totalSec = Math.floor(ms / 1000);
        var h = Math.floor(totalSec / 3600);
        var m = Math.floor((totalSec % 3600) / 60);
        var s = totalSec % 60;
        var mm = (m < 10 && h > 0 ? "0" : "") + m;
        var ss = (s < 10 ? "0" : "") + s;
        return h > 0 ? h + ":" + mm + ":" + ss : m + ":" + ss;
    }

    function askFor(data, posMs, durMs) {
        payload = data;
        positionMs = posMs;
        durationMs = durMs;
        open();
    }

    // Keyboard control: Continue is focused on open; arrows move between
    // the two buttons, Enter/Space activates, Escape closes.
    onOpened: continueBtn.forceActiveFocus()

    contentItem: ColumnLayout {
        spacing: 18

        Text {
            text: "Continue Watching?"
            color: "#FFD54F"
            font.pixelSize: 18
            font.bold: true
        }

        Text {
            text: "You stopped \"" + (root.payload ? root.payload.channelName : "")
                  + "\" at " + root.formatTime(root.positionMs)
                  + (root.durationMs > 0 ? " of " + root.formatTime(root.durationMs) : "") + "."
            color: "#DDDDDD"
            font.pixelSize: 13
            lineHeight: 1.2
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
        }

        // Watched-progress bar
        Rectangle {
            visible: root.durationMs > 0
            Layout.fillWidth: true
            height: 4
            radius: 2
            color: "#333333"
            Rectangle {
                width: parent.width * Math.min(1, root.positionMs / Math.max(1, root.durationMs))
                height: parent.height
                radius: 2
                color: "#FFD54F"
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.topMargin: 4
            spacing: 12
            Item { Layout.fillWidth: true }

            Button {
                id: startOverBtn
                text: "Start Over"
                KeyNavigation.right: continueBtn
                KeyNavigation.left: continueBtn
                Keys.onReturnPressed: startOverBtn.clicked()
                Keys.onEnterPressed: startOverBtn.clicked()
                background: Rectangle {
                    color: startOverBtn.hovered || startOverBtn.activeFocus ? "#2A2A2A" : "transparent"
                    radius: 6; implicitHeight: 38; implicitWidth: 110
                    border.color: startOverBtn.activeFocus ? "#FFD54F" : "#444444"
                    border.width: startOverBtn.activeFocus ? 2 : 1
                }
                contentItem: Text {
                    text: startOverBtn.text
                    color: startOverBtn.hovered || startOverBtn.activeFocus ? "#FFFFFF" : "#AAAAAA"
                    font.pixelSize: 14
                    horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter
                }
                onClicked: {
                    root.close();
                    root.startOverChosen(root.payload);
                }
            }

            Button {
                id: continueBtn
                text: "▶ Continue"
                KeyNavigation.left: startOverBtn
                KeyNavigation.right: startOverBtn
                Keys.onReturnPressed: continueBtn.clicked()
                Keys.onEnterPressed: continueBtn.clicked()
                background: Rectangle {
                    color: continueBtn.hovered || continueBtn.activeFocus ? "#FFE082" : "#FFD54F"
                    radius: 6; implicitHeight: 38; implicitWidth: 130
                    border.color: continueBtn.activeFocus ? "#FFFFFF" : "transparent"
                    border.width: continueBtn.activeFocus ? 2 : 0
                }
                contentItem: Text {
                    text: continueBtn.text; color: "#121212"; font.bold: true; font.pixelSize: 14
                    horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter
                }
                onClicked: {
                    root.close();
                    root.continueChosen(root.payload);
                }
            }
        }
    }
}
