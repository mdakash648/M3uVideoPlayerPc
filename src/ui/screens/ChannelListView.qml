import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    color: "#121212"

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 20
        spacing: 20

        // Header and Search
        RowLayout {
            Layout.fillWidth: true
            
            Button {
                text: "← Back"
                background: Rectangle { color: "transparent" }
                contentItem: Text { text: parent.text; color: "#FFD54F"; font.bold: true; font.pixelSize: 16 }
            }

            Text {
                text: qsTr("Channels")
                color: "#FFFFFF"
                font.pixelSize: 24
                font.bold: true
                Layout.leftMargin: 15
            }
            
            Item { Layout.fillWidth: true } // spacer
            
            // View mode toggle placeholder
            RowLayout {
                spacing: 5
                Repeater {
                    model: ["List", "Grid", "Title", "Poster"]
                    Button {
                        text: modelData
                        background: Rectangle { color: index === 0 ? "#FFD54F" : "#1E1E1E"; radius: 4; implicitHeight: 30; implicitWidth: 60 }
                        contentItem: Text { text: parent.text; color: index === 0 ? "#121212" : "#FFFFFF"; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter; font.pixelSize: 12 }
                    }
                }
            }
            
            TextField {
                placeholderText: "Search channel..."
                color: "#FFFFFF"
                background: Rectangle { color: "#1E1E1E"; radius: 6; implicitHeight: 40; implicitWidth: 200; border.color: "#333333" }
                leftPadding: 10
            }
        }

        // List View for Channels (List Mode Default)
        ListView {
            id: channelList
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            spacing: 10
            
            model: 20 // Dummy channels
            
            delegate: Rectangle {
                width: ListView.view.width
                height: 60
                color: "#1E1E1E"
                radius: 8
                border.color: "#333333"
                border.width: 1
                
                RowLayout {
                    anchors.fill: parent
                    anchors.margins: 10
                    spacing: 15
                    
                    // Logo Placeholder
                    Rectangle {
                        width: 60; height: 40
                        color: "#2A2A2A"
                        radius: 4
                        Text { anchors.centerIn: parent; text: "LOGO"; color: "#555555"; font.pixelSize: 12 }
                    }
                    
                    Text {
                        text: "Channel " + (index + 1)
                        color: "#FFFFFF"
                        font.pixelSize: 16
                        font.bold: true
                        Layout.fillWidth: true
                    }
                    
                    Button {
                        text: "★"
                        background: Rectangle { color: "transparent" }
                        contentItem: Text { text: parent.text; color: index % 3 === 0 ? "#FFD54F" : "#555555"; font.pixelSize: 20 }
                    }
                    
                    Button {
                        text: "Play"
                        background: Rectangle { color: "#FFD54F"; radius: 6; implicitHeight: 35; implicitWidth: 80 }
                        contentItem: Text { text: parent.text; color: "#121212"; font.bold: true; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                    }
                }
                
                MouseArea {
                    anchors.fill: parent
                    hoverEnabled: true
                    onEntered: parent.border.color = "#FFD54F"
                    onExited: parent.border.color = "#333333"
                }
            }
        }
    }
}
