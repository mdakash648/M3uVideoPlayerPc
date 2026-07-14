import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Popup {
    id: dialog
    width: 500
    height: 480
    modal: true
    focus: true
    anchors.centerIn: parent
    
    background: Rectangle {
        color: "#1E1E1E"
        radius: 12
        border.color: "#333333"
        border.width: 1
    }
    
    property int currentTab: 0 // 0: M3U, 1: Xtream Codes
    
    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 25
        spacing: 20
        
        Text {
            text: "Add New Playlist"
            color: "#FFFFFF"
            font.pixelSize: 22
            font.bold: true
        }
        
        // Tab Headers
        RowLayout {
            spacing: 10
            
            Button {
                text: "M3U Link / File"
                Layout.fillWidth: true
                background: Rectangle {
                    color: dialog.currentTab === 0 ? "#FFD54F" : "#2A2A2A"
                    radius: 6
                    implicitHeight: 40
                }
                contentItem: Text {
                    text: parent.text
                    color: dialog.currentTab === 0 ? "#121212" : "#FFFFFF"
                    font.bold: true
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                onClicked: dialog.currentTab = 0
            }
            
            Button {
                text: "Xtream Codes API"
                Layout.fillWidth: true
                background: Rectangle {
                    color: dialog.currentTab === 1 ? "#FFD54F" : "#2A2A2A"
                    radius: 6
                    implicitHeight: 40
                }
                contentItem: Text {
                    text: parent.text
                    color: dialog.currentTab === 1 ? "#121212" : "#FFFFFF"
                    font.bold: true
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                onClicked: dialog.currentTab = 1
            }
        }
        
        // Form Fields (Dynamic based on Tab)
        ColumnLayout {
            Layout.fillWidth: true
            spacing: 15
            
            // Common Name Field
            TextField {
                id: nameField
                Layout.fillWidth: true
                placeholderText: "Playlist Name"
                color: "#FFFFFF"
                background: Rectangle { color: "#121212"; radius: 6; implicitHeight: 40; border.color: nameField.activeFocus ? "#FFD54F" : "#333333" }
                leftPadding: 10
            }
            
            // M3U Tab Specific
            TextField {
                id: urlField
                visible: dialog.currentTab === 0
                Layout.fillWidth: true
                placeholderText: "M3U URL or local file path"
                color: "#FFFFFF"
                background: Rectangle { color: "#121212"; radius: 6; implicitHeight: 40; border.color: urlField.activeFocus ? "#FFD54F" : "#333333" }
                leftPadding: 10
            }
            
            // Xtream Tab Specific
            TextField {
                id: xtreamUrlField
                visible: dialog.currentTab === 1
                Layout.fillWidth: true
                placeholderText: "Server URL (e.g. http://server:port)"
                color: "#FFFFFF"
                background: Rectangle { color: "#121212"; radius: 6; implicitHeight: 40; border.color: xtreamUrlField.activeFocus ? "#FFD54F" : "#333333" }
                leftPadding: 10
            }
            TextField {
                id: usernameField
                visible: dialog.currentTab === 1
                Layout.fillWidth: true
                placeholderText: "Username"
                color: "#FFFFFF"
                background: Rectangle { color: "#121212"; radius: 6; implicitHeight: 40; border.color: usernameField.activeFocus ? "#FFD54F" : "#333333" }
                leftPadding: 10
            }
            TextField {
                id: passwordField
                visible: dialog.currentTab === 1
                Layout.fillWidth: true
                placeholderText: "Password"
                echoMode: TextInput.Password
                color: "#FFFFFF"
                background: Rectangle { color: "#121212"; radius: 6; implicitHeight: 40; border.color: passwordField.activeFocus ? "#FFD54F" : "#333333" }
                leftPadding: 10
            }
        }
        
        Item { Layout.fillHeight: true } // Spacer
        
        // Action Buttons
        RowLayout {
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignRight
            spacing: 15
            
            Button {
                text: "Cancel"
                background: Rectangle { color: "transparent"; border.color: "#555555"; radius: 6; implicitHeight: 40; implicitWidth: 100 }
                contentItem: Text { text: parent.text; color: "#FFFFFF"; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                onClicked: dialog.close()
            }
            
            Button {
                text: "Add Playlist"
                background: Rectangle { color: "#FFD54F"; radius: 6; implicitHeight: 40; implicitWidth: 120 }
                contentItem: Text { text: parent.text; color: "#121212"; font.bold: true; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                onClicked: {
                    // TODO: Pass data to C++ backend
                    console.log("Adding playlist...")
                    dialog.close()
                }
            }
        }
    }
}
