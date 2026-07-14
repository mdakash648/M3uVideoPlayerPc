import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: root
    width: 250
    color: "#1E1E1E" // Surface color

    signal navigationRequested(string page)

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 10
        spacing: 15

        Text {
            text: "M3U Player"
            color: "#FFD54F" // Primary accent
            font.pixelSize: 24
            font.bold: true
            Layout.alignment: Qt.AlignHCenter
            Layout.bottomMargin: 20
        }

        SidebarButton {
            text: "Playlists"
            iconName: "list"
            onClicked: root.navigationRequested("Playlists")
            Layout.fillWidth: true
        }

        SidebarButton {
            text: "Direct Link"
            iconName: "link"
            onClicked: root.navigationRequested("DirectLink")
            Layout.fillWidth: true
        }

        SidebarButton {
            text: "History"
            iconName: "history"
            onClicked: root.navigationRequested("History")
            Layout.fillWidth: true
        }

        Item {
            Layout.fillHeight: true // Spacer
        }

        SidebarButton {
            text: "Settings"
            iconName: "settings"
            onClicked: root.navigationRequested("Settings")
            Layout.fillWidth: true
        }
    }
}
