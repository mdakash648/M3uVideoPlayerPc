import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../components" as Components

Rectangle {
    color: "#121212"

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 20

        RowLayout {
            Layout.fillWidth: true
            
            Text {
                text: qsTr("My Playlists")
                color: "#FFFFFF"
                font.pixelSize: 28
                font.bold: true
                Layout.fillWidth: true
            }
            
            Button {
                id: addPlaylistBtn
                text: "+ Add Playlist"
                background: Rectangle {
                    color: "#FFD54F"
                    radius: 6
                    implicitHeight: 40
                    implicitWidth: 120
                }
                contentItem: Text {
                    text: parent.text
                    color: addPlaylistBtn.hovered ? "#FFFFFF" : "#121212"
                    font.bold: true
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    Behavior on color { ColorAnimation { duration: 150 } }
                }
                onClicked: addPlaylistDialog.open()
            }
        }

        Components.AddPlaylistDialog {
            id: addPlaylistDialog
        }

        // Placeholder for list view
        ListView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            
            model: 5 // Dummy data
            
            delegate: Rectangle {
                width: ListView.view.width
                height: 80
                color: "#1E1E1E"
                radius: 10
                border.color: "#333333"
                border.width: 1
                
                RowLayout {
                    anchors.fill: parent
                    anchors.margins: 15
                    
                    ColumnLayout {
                        Layout.fillWidth: true
                        Text {
                            text: "Playlist " + (index + 1)
                            color: "#FFFFFF"
                            font.pixelSize: 18
                            font.bold: true
                        }
                        Text {
                            text: "Updated: Just now"
                            color: "#AAAAAA"
                            font.pixelSize: 12
                        }
                    }
                    
                    Button {
                        text: "Play"
                        background: Rectangle {
                            color: "#FFD54F"
                            radius: 6
                            implicitWidth: 80
                            implicitHeight: 35
                        }
                        contentItem: Text {
                            text: parent.text
                            color: "#121212"
                            font.bold: true
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                    }
                }
                
                MouseArea {
                    anchors.fill: parent
                    hoverEnabled: true
                    onEntered: parent.border.color = "#FFD54F"
                    onExited: parent.border.color = "#333333"
                }
            }
            
            spacing: 15
        }
    }
}
