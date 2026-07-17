import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import M3uVideoPlayer

Popup {
    id: dialog
    width: 500
    height: 540
    modal: true
    focus: true
    anchors.centerIn: parent
    
    background: Rectangle {
        color: "#0d141d"
        radius: 12
        border.color: "#2C313A"
        border.width: 1
    }
    
    property int currentTab: 0 // 0: M3U, 1: Xtream Codes
    
    ColumnLayout {
        anchors.fill: parent
        spacing: 0
        
        // Header
        RowLayout {
            Layout.fillWidth: true
            Layout.margins: 24
            Layout.bottomMargin: 16
            
            Text {
                text: "Add New Playlist"
                color: "#dce3f0"
                font.pixelSize: 20
                font.bold: true
                Layout.fillWidth: true
            }
            
            Button {
                text: "✕"
                background: null
                contentItem: Text {
                    text: parent.text
                    color: parent.hovered ? "#ffb781" : "#dec1ae"
                    font.pixelSize: 18
                    horizontalAlignment: Text.AlignRight
                }
                onClicked: dialog.close()
            }
        }
        
        // Tabs
        ColumnLayout {
            Layout.fillWidth: true
            Layout.leftMargin: 24
            Layout.rightMargin: 24
            spacing: 0
            
            RowLayout {
                Layout.fillWidth: true
                spacing: 0
                
                Button {
                    text: "M3U Link / File"
                    Layout.fillWidth: true
                    background: Rectangle {
                        color: "transparent"
                        Rectangle {
                            anchors.bottom: parent.bottom
                            width: parent.width
                            height: 2
                            color: dialog.currentTab === 0 ? "#ff8800" : "transparent"
                        }
                    }
                    contentItem: Text {
                        text: parent.text
                        color: dialog.currentTab === 0 ? "#ffb781" : "#dec1ae"
                        font.bold: true
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                        font.pixelSize: 14
                    }
                    onClicked: dialog.currentTab = 0
                }
                
                Button {
                    text: "Xtream Codes API"
                    Layout.fillWidth: true
                    background: Rectangle {
                        color: "transparent"
                        Rectangle {
                            anchors.bottom: parent.bottom
                            width: parent.width
                            height: 2
                            color: dialog.currentTab === 1 ? "#ff8800" : "transparent"
                        }
                    }
                    contentItem: Text {
                        text: parent.text
                        color: dialog.currentTab === 1 ? "#ffb781" : "#dec1ae"
                        font.bold: true
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                        font.pixelSize: 14
                    }
                    onClicked: dialog.currentTab = 1
                }
            }
            
            Rectangle {
                Layout.fillWidth: true
                height: 1
                color: "#2C313A"
            }
        }
        
        // Form Fields
        ColumnLayout {
            Layout.fillWidth: true
            Layout.margins: 24
            spacing: 16
            
            // Common Name Field
            ColumnLayout {
                spacing: 6
                Layout.fillWidth: true
                Text { text: "Playlist Name"; color: "#dec1ae"; font.pixelSize: 13; font.weight: Font.Medium }
                TextField {
                    id: nameField
                    Layout.fillWidth: true
                    placeholderText: "e.g. My Favorite Channels"
                    placeholderTextColor: "#43474d"
                    color: "#dce3f0"
                    background: Rectangle { color: "#151c26"; radius: 8; implicitHeight: 44; border.color: nameField.activeFocus ? "#ff8800" : "#2C313A" }
                    leftPadding: 16
                }
            }
            
            // M3U Tab Specific
            ColumnLayout {
                visible: dialog.currentTab === 0
                Layout.fillWidth: true
                spacing: 6
                Text { text: "M3U URL or Path"; color: "#dec1ae"; font.pixelSize: 13; font.weight: Font.Medium }
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8
                    TextField {
                        id: urlField
                        Layout.fillWidth: true
                        placeholderText: "http://example.com/playlist.m3u"
                        placeholderTextColor: "#43474d"
                        color: "#dce3f0"
                        background: Rectangle { color: "#151c26"; radius: 8; implicitHeight: 44; border.color: urlField.activeFocus ? "#ff8800" : "#2C313A" }
                        leftPadding: 16
                    }
                    Button {
                        text: "Browse"
                        implicitHeight: 44
                        background: Rectangle { color: parent.hovered ? "#333a44" : "#2e3540"; radius: 8; border.color: "#2C313A"; border.width: 1 }
                        contentItem: Text { text: parent.text; color: "#dce3f0"; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter; font.pixelSize: 14 }
                        onClicked: fileDialog.open()
                    }
                }
                Text { text: "Paste a URL or browse for a local .m3u file."; color: "#dec1ae"; font.pixelSize: 12; opacity: 0.7 }
            }
            
            // Xtream Tab Specific
            ColumnLayout {
                visible: dialog.currentTab === 1
                Layout.fillWidth: true
                spacing: 6
                Text { text: "Server URL"; color: "#dec1ae"; font.pixelSize: 13; font.weight: Font.Medium }
                TextField {
                    id: xtreamUrlField
                    Layout.fillWidth: true
                    placeholderText: "e.g. http://server:port"
                    placeholderTextColor: "#43474d"
                    color: "#dce3f0"
                    background: Rectangle { color: "#151c26"; radius: 8; implicitHeight: 44; border.color: xtreamUrlField.activeFocus ? "#ff8800" : "#2C313A" }
                    leftPadding: 16
                }
            }
            RowLayout {
                visible: dialog.currentTab === 1
                Layout.fillWidth: true
                spacing: 16
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 6
                    Text { text: "Username"; color: "#dec1ae"; font.pixelSize: 13; font.weight: Font.Medium }
                    TextField {
                        id: usernameField
                        Layout.fillWidth: true
                        placeholderText: "Username"
                        placeholderTextColor: "#43474d"
                        color: "#dce3f0"
                        background: Rectangle { color: "#151c26"; radius: 8; implicitHeight: 44; border.color: usernameField.activeFocus ? "#ff8800" : "#2C313A" }
                        leftPadding: 16
                    }
                }
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 6
                    Text { text: "Password"; color: "#dec1ae"; font.pixelSize: 13; font.weight: Font.Medium }
                    TextField {
                        id: passwordField
                        Layout.fillWidth: true
                        placeholderText: "Password"
                        placeholderTextColor: "#43474d"
                        echoMode: TextInput.Password
                        color: "#dce3f0"
                        background: Rectangle { color: "#151c26"; radius: 8; implicitHeight: 44; border.color: passwordField.activeFocus ? "#ff8800" : "#2C313A" }
                        leftPadding: 16
                    }
                }
            }
        }
        
        Item { Layout.fillHeight: true } // Spacer
        
        // Footer (Action Buttons)
        Rectangle {
            Layout.fillWidth: true
            height: 1
            color: "#2C313A"
        }
        
        Rectangle {
            Layout.fillWidth: true
            height: 72
            color: "#111721" // Slightly different background for footer like surface-container/30
            radius: 12
            // Hide top corners radius by overlaying a rectangle or just rely on clipping (parent doesn't clip, so just don't set top radius)
            
            RowLayout {
                anchors.fill: parent
                anchors.margins: 16
                anchors.rightMargin: 24
                Layout.alignment: Qt.AlignRight
                spacing: 16
                
                Item { Layout.fillWidth: true } // push buttons to right
                
                Button {
                    text: "Cancel"
                    background: Rectangle { color: "transparent"; radius: 4; implicitHeight: 40; implicitWidth: 100 }
                    contentItem: Text { text: parent.text; color: parent.hovered ? "#dce3f0" : "#dec1ae"; font.pixelSize: 14; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                    onClicked: dialog.close()
                }
                
                Button {
                    text: "Add Playlist"
                    background: Rectangle { color: "#FF8800"; radius: 4; implicitHeight: 40; implicitWidth: 120; opacity: parent.hovered ? 0.9 : 1.0 }
                    contentItem: Text { text: parent.text; color: "#000000"; font.bold: true; font.pixelSize: 14; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                    onClicked: {
                        var name = nameField.text;
                        if (dialog.currentTab === 0) {
                            var url = urlField.text;
                            if (url.startsWith("file:///")) {
                                url = url.replace("file:///", "");
                            }
                            AppController.playlistViewModel.addPlaylistAsync(name, url, false, "", "");
                        } else {
                            AppController.playlistViewModel.addPlaylistAsync(name, xtreamUrlField.text, true, usernameField.text, passwordField.text);
                        }
                        dialog.close()
                    }
                }
            }
        }
    }
    
    FileDialog {
        id: fileDialog
        title: "Please choose an M3U file"
        nameFilters: ["M3U files (*.m3u *.m3u8)", "All files (*)"]
        onAccepted: {
            urlField.text = fileDialog.selectedFile
        }
    }
}
