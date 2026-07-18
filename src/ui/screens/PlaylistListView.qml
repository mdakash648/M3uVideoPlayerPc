import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import M3uVideoPlayer
import "../components" as Components

Rectangle {
    color: "#0d141d" // Surface color

    signal playlistOpened(int playlistId, string playlistName)

    function focusHeader() {
        addPlaylistBtn.forceActiveFocus();
    }

    function focusMain() {
        if (grid.currentIndex < 0 && grid.count > 0) grid.currentIndex = 0;
        grid.forceActiveFocus();
    }

    StackView.onActivated: focusMain()

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // Top AppBar
        Rectangle {
            Layout.fillWidth: true
            height: 64
            color: "#0d141d"
            border.color: "#2C313A"
            border.width: 1
            
            RowLayout {
                anchors.fill: parent
                anchors.margins: 24
                
                Text {
                    text: qsTr("My Playlists")
                    color: "#ffb781" // primary
                    font.pixelSize: 20
                    font.bold: true
                    Layout.fillWidth: true
                }
                
                Button {
                    id: addPlaylistBtn
                    text: "Add Playlist"
                    focus: true
                    Keys.onReturnPressed: addPlaylistBtn.clicked()
                    Keys.onEnterPressed: addPlaylistBtn.clicked()
                    background: Rectangle {
                        color: "#FF8800"
                        radius: 4
                        implicitHeight: 36
                        implicitWidth: 120
                        border.color: addPlaylistBtn.activeFocus ? "#FFFFFF" : "transparent"
                        border.width: addPlaylistBtn.activeFocus ? 2 : 0
                    }
                    contentItem: RowLayout {
                        spacing: 6
                        Text {
                            text: "+"
                            color: "#000000"
                            font.bold: true
                            font.pixelSize: 18
                        }
                        Text {
                            text: addPlaylistBtn.text
                            color: "#000000"
                            font.bold: true
                            font.pixelSize: 14
                        }
                    }
                    onClicked: addPlaylistDialog.open()
                }
            }
        }

        Components.AddPlaylistDialog {
            id: addPlaylistDialog
        }

        // Body Canvas
        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.margins: 24
            spacing: 24

            // Sub-header
            RowLayout {
                Layout.fillWidth: true
                Text {
                    text: "Active Playlists"
                    color: "#dce3f0"
                    font.pixelSize: 20
                    font.bold: true
                    Layout.fillWidth: true
                }
                
                RowLayout {
                    spacing: 16
                    Text { text: "↿⇂"; color: "#dec1ae"; font.pixelSize: 16 }
                    Text { text: "≡"; color: "#dec1ae"; font.pixelSize: 16 }
                }
            }

            // Grid View
            GridView {
                id: grid
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true
                focus: true
                KeyNavigation.up: addPlaylistBtn
                
                cellWidth: 320
                cellHeight: 96
                
                model: AppController.playlistViewModel
                
                delegate: Item {
                    width: grid.cellWidth
                    height: grid.cellHeight

                    Keys.onPressed: (event) => {
                        if (event.key === Qt.Key_Enter || event.key === Qt.Key_Return || event.key === Qt.Key_Space) {
                            var pId = model.id !== undefined ? model.id : -1;
                            var pName = model.name !== undefined ? model.name : ("Playlist " + (index + 1));
                            playlistOpened(pId, pName);
                            event.accepted = true;
                        }
                    }
                    
                    Rectangle {
                        anchors.fill: parent
                        anchors.margins: 8
                        property bool isActiveItem: grid.activeFocus && parent.GridView.isCurrentItem
                        color: (hoverArea.containsMouse || isActiveItem) ? "#2e3540" : "#151c26"
                        radius: 12
                        border.color: (hoverArea.containsMouse || isActiveItem) ? "#ffb781" : "#2C313A"
                        border.width: isActiveItem ? 2 : 1
                        
                        Behavior on border.color { ColorAnimation { duration: 200 } }
                        Behavior on color { ColorAnimation { duration: 200 } }
                        
                        MouseArea {
                            id: hoverArea
                            anchors.fill: parent
                            hoverEnabled: true
                            onClicked: {
                                grid.currentIndex = index;
                                var pId = model.id !== undefined ? model.id : -1;
                                var pName = model.name !== undefined ? model.name : ("Playlist " + (index + 1));
                                playlistOpened(pId, pName);
                            }
                        }
                        
                        RowLayout {
                            anchors.fill: parent
                            anchors.margins: 16
                            spacing: 16
                            
                            // Folder Icon Container
                            Rectangle {
                                width: 48
                                height: 48
                                radius: 8
                                color: "#1AFFFFFF" // 10% white over dark
                                
                                Components.FolderIcon {
                                    anchors.centerIn: parent
                                    size: 28
                                }
                            }
                            
                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 2
                                Text {
                                    text: model.name ? model.name : ("Playlist " + (index + 1))
                                    color: hoverArea.containsMouse ? "#ffb781" : "#dce3f0"
                                    font.pixelSize: 16
                                    font.bold: true
                                    elide: Text.ElideRight
                                    Layout.fillWidth: true
                                    Behavior on color { ColorAnimation { duration: 200 } }
                                }
                                Text {
                                    text: (model.type ? "Xtream Codes" : "M3U")
                                    color: "#dec1ae"
                                    font.pixelSize: 12
                                }
                            }
                            // Playlist deletion intentionally lives only in
                            // Settings → Playlists — no delete button here.
                        }
                    }
                }
            }
        }
    }
}
