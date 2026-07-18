import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtCore

Rectangle {
    id: root
    property bool isCollapsed: false
    width: isCollapsed ? 65 : 250
    color: "#1E1E1E" // Surface color

    Behavior on width {
        NumberAnimation { duration: 200; easing.type: Easing.InOutQuad }
    }

    Settings {
        category: "Sidebar"
        property alias isCollapsed: root.isCollapsed
    }

    signal navigationRequested(string page)

    function focusFirstItem() {
        btnPlaylists.forceActiveFocus();
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 10
        spacing: 15

        RowLayout {
            Layout.fillWidth: true
            Layout.bottomMargin: 20

            Button {
                id: btnMenu
                text: "☰"
                Layout.preferredWidth: 45
                Layout.preferredHeight: 45
                padding: 0
                KeyNavigation.down: btnPlaylists
                Keys.onReturnPressed: btnMenu.clicked()
                Keys.onEnterPressed: btnMenu.clicked()
                background: Rectangle { 
                    color: btnMenu.activeFocus ? "#333333" : "transparent"
                    radius: 8 
                    border.color: btnMenu.activeFocus ? "#FFD54F" : "transparent"
                    border.width: btnMenu.activeFocus ? 1 : 0
                }
                contentItem: Item {
                    Text { 
                        text: parent.parent.text
                        color: "#FFD54F"
                        font.pixelSize: 20
                        anchors.verticalCenter: parent.verticalCenter
                        x: root.isCollapsed ? (parent.width - width) / 2 : 10
                        
                        Behavior on x {
                            NumberAnimation { duration: 200; easing.type: Easing.InOutQuad }
                        }
                    }
                }
                onClicked: root.isCollapsed = !root.isCollapsed
            }

            Text {
                text: "M3U Player"
                color: "#FFD54F" // Primary accent
                font.pixelSize: 20
                font.bold: true
                visible: !root.isCollapsed
                Layout.alignment: Qt.AlignVCenter
                Layout.fillWidth: true
            }
        }

        SidebarButton {
            id: btnPlaylists
            text: "Playlists"
            iconName: "list"
            isCollapsed: root.isCollapsed
            KeyNavigation.up: btnMenu
            KeyNavigation.down: btnDirectLink
            onClicked: root.navigationRequested("Playlists")
            Layout.fillWidth: true
        }

        SidebarButton {
            id: btnDirectLink
            text: "Direct Link"
            iconName: "link"
            isCollapsed: root.isCollapsed
            KeyNavigation.up: btnPlaylists
            KeyNavigation.down: btnSettings
            onClicked: root.navigationRequested("DirectLink")
            Layout.fillWidth: true
        }

        Item {
            Layout.fillHeight: true // Spacer
        }

        SidebarButton {
            id: btnSettings
            text: "Settings"
            iconName: "settings"
            isCollapsed: root.isCollapsed
            KeyNavigation.up: btnDirectLink
            onClicked: root.navigationRequested("Settings")
            Layout.fillWidth: true
        }
    }
}
