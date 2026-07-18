import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import M3uVideoPlayer

Rectangle {
    color: "#121212"

    signal playRequested(string streamUrl, string referer, string userAgent)

    Component.onCompleted: checkClipboard()
    onVisibleChanged: if (visible) checkClipboard()

    function checkClipboard() {
        var text = AppController.getClipboardText().trim();
        if ((text.startsWith("http://") || text.startsWith("https://")) && urlInput.text === "") {
            urlInput.text = text;
        }
    }

    ColumnLayout {
        anchors.centerIn: parent
        width: Math.min(600, parent.width * 0.8)
        spacing: 20

        Text {
            text: qsTr("Play Direct Stream")
            color: "#FFFFFF"
            font.pixelSize: 28
            font.bold: true
            Layout.alignment: Qt.AlignHCenter
        }

        Text {
            text: qsTr("Paste an M3U8, MP4, or TS stream URL below to start playing immediately.")
            color: "#AAAAAA"
            font.pixelSize: 14
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
            horizontalAlignment: Text.AlignHCenter
        }

        TextField {
            id: urlInput
            Layout.fillWidth: true
            placeholderText: "https://example.com/stream.m3u8"
            color: "#FFFFFF"
            font.pixelSize: 16
            
            background: Rectangle {
                color: "#1E1E1E"
                radius: 8
                border.color: urlInput.activeFocus ? "#FFD54F" : "#333333"
                border.width: 1
                implicitHeight: 50
            }
            leftPadding: 15
            rightPadding: 15
        }

        TextField {
            id: refererInput
            Layout.fillWidth: true
            placeholderText: "HTTP Referer (Optional) e.g., https://example.com/"
            color: "#FFFFFF"
            font.pixelSize: 16
            
            background: Rectangle {
                color: "#1E1E1E"
                radius: 8
                border.color: refererInput.activeFocus ? "#FFD54F" : "#333333"
                border.width: 1
                implicitHeight: 50
            }
            leftPadding: 15
            rightPadding: 15
        }

        TextField {
            id: userAgentInput
            Layout.fillWidth: true
            placeholderText: "User-Agent (Optional)"
            color: "#FFFFFF"
            font.pixelSize: 16
            
            background: Rectangle {
                color: "#1E1E1E"
                radius: 8
                border.color: userAgentInput.activeFocus ? "#FFD54F" : "#333333"
                border.width: 1
                implicitHeight: 50
            }
            leftPadding: 15
            rightPadding: 15
        }

        Button {
            text: "Play Stream"
            Layout.alignment: Qt.AlignHCenter
            Layout.topMargin: 10

            background: Rectangle {
                color: "#FFD54F"
                radius: 8
                implicitWidth: 150
                implicitHeight: 45
            }
            contentItem: Text {
                text: parent.text
                color: "#121212"
                font.bold: true
                font.pixelSize: 16
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }

            onClicked: {
                if (urlInput.text.trim() !== "") {
                    playRequested(urlInput.text.trim(), refererInput.text.trim(), userAgentInput.text.trim());
                }
            }
        }

        // ----- Or import a local playlist file -----
        RowLayout {
            Layout.fillWidth: true
            Layout.topMargin: 10
            spacing: 12

            Rectangle { Layout.fillWidth: true; height: 1; color: "#333333" }
            Text {
                text: qsTr("OR")
                color: "#777777"
                font.pixelSize: 12
                font.bold: true
            }
            Rectangle { Layout.fillWidth: true; height: 1; color: "#333333" }
        }

        Button {
            id: openFileBtn
            text: qsTr("Open M3U / M3U8 File…")
            Layout.alignment: Qt.AlignHCenter

            background: Rectangle {
                color: openFileBtn.hovered ? "#2A2A2A" : "#1E1E1E"
                radius: 8
                border.color: "#FFD54F"
                border.width: 1
                implicitWidth: 220
                implicitHeight: 45
            }
            contentItem: Text {
                text: openFileBtn.text
                color: "#FFD54F"
                font.bold: true
                font.pixelSize: 15
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }

            onClicked: m3uFileDialog.open()
        }

        Text {
            text: qsTr("All videos from the file will be added to the \"History\" playlist (keeping their group, referer and user-agent).")
            color: "#777777"
            font.pixelSize: 12
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
            horizontalAlignment: Text.AlignHCenter
        }
    }

    FileDialog {
        id: m3uFileDialog
        title: qsTr("Select an M3U playlist file")
        nameFilters: ["M3U playlists (*.m3u *.m3u8)", "All files (*)"]
        onAccepted: AppController.openM3uFile(selectedFile.toString())
    }
}
