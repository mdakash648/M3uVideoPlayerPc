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
                // onClicked: go back
            }

            Text {
                text: qsTr("Groups / Categories")
                color: "#FFFFFF"
                font.pixelSize: 24
                font.bold: true
                Layout.leftMargin: 15
            }
            
            Item { Layout.fillWidth: true } // spacer
            
            TextField {
                placeholderText: "Search categories..."
                color: "#FFFFFF"
                background: Rectangle { color: "#1E1E1E"; radius: 6; implicitHeight: 40; implicitWidth: 250; border.color: "#333333" }
                leftPadding: 10
            }
        }

        // Grid View for Groups
        GridView {
            id: groupGrid
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            
            cellWidth: (width / 3) - 10
            cellHeight: 120
            
            model: 12 // Dummy groups
            
            delegate: Rectangle {
                width: groupGrid.cellWidth - 15
                height: groupGrid.cellHeight - 15
                color: "#1E1E1E"
                radius: 12
                border.color: "#333333"
                border.width: 1
                
                ColumnLayout {
                    anchors.centerIn: parent
                    
                    // Folder Icon placeholder
                    Rectangle {
                        width: 40; height: 30
                        color: "#FFD54F"
                        radius: 4
                        Layout.alignment: Qt.AlignHCenter
                    }
                    
                    Text {
                        text: "Category " + (index + 1)
                        color: "#FFFFFF"
                        font.pixelSize: 16
                        font.bold: true
                        Layout.alignment: Qt.AlignHCenter
                    }
                    
                    Text {
                        text: "150 Channels"
                        color: "#AAAAAA"
                        font.pixelSize: 12
                        Layout.alignment: Qt.AlignHCenter
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
