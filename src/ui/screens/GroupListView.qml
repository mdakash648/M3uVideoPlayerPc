import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../components"
import M3uVideoPlayer

Rectangle {
    color: "#121212"

    property int playlistId: -1
    property string playlistName: "Groups / Categories"
    property string viewMode: "Grid" // "Grid" or "List"

    signal backRequested()
    signal groupOpened(int groupId, string groupName)
    signal allChannelsOpened()
    signal favoritesOpened()

    onPlaylistIdChanged: {
        if (playlistId !== -1) {
            AppController.groupViewModel.loadGroups(playlistId);
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 20
        spacing: 20

        // Header and Search
        RowLayout {
            Layout.fillWidth: true

            Button {
                text: "← Back"
                background: Rectangle { color: "transparent"; implicitWidth: 60; implicitHeight: 30 }
                contentItem: Text { text: parent.text; color: "#FFD54F"; font.bold: true; font.pixelSize: 16; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                onClicked: backRequested()
            }

            Text {
                text: playlistName
                color: "#FFFFFF"
                font.pixelSize: 24
                font.bold: true
                Layout.leftMargin: 15
            }

            Item { Layout.fillWidth: true } // spacer

            ViewModeToggle {
                modes: ["Grid", "List"]
                currentMode: viewMode
                onModeSelected: (mode) => viewMode = mode
            }

            SortMenuButton {
                onSortSelected: (mode) => AppController.groupViewModel.setSortMode(mode)
            }

            TextField {
                placeholderText: "Search categories..."
                color: "#FFFFFF"
                background: Rectangle { color: "#1E1E1E"; radius: 6; implicitHeight: 40; implicitWidth: 220; border.color: "#333333" }
                leftPadding: 10
                onTextChanged: AppController.groupViewModel.setSearchQuery(text)
            }
        }



        // Grid View for Groups
        GridView {
            id: groupGrid
            visible: viewMode === "Grid"
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true

            cellWidth: (width / 3) - 10
            cellHeight: 120

            model: AppController.groupViewModel

            delegate: Rectangle {
                width: groupGrid.cellWidth - 15
                height: groupGrid.cellHeight - 15
                color: "#1E1E1E"
                radius: 12
                border.color: "#333333"
                border.width: 1

                ColumnLayout {
                    anchors.centerIn: parent

                    // Windows-style folder icon
                    FolderIcon {
                        visible: model.id !== -200
                        size: 46
                        Layout.alignment: Qt.AlignHCenter
                    }

                    Rectangle {
                        visible: model.id === -200
                        width: 46; height: 46
                        radius: 8
                        color: "#2A2A2A"
                        Layout.alignment: Qt.AlignHCenter
                        Text { anchors.centerIn: parent; text: "★"; color: "#FFD54F"; font.pixelSize: 24 }
                    }

                    Text {
                        text: model.name ? model.name : ("Category " + (index + 1))
                        color: "#FFFFFF"
                        font.pixelSize: 16
                        font.bold: true
                        Layout.alignment: Qt.AlignHCenter
                    }

                    Text {
                        text: (model.channelCount !== undefined ? model.channelCount : 0) + " Channels"
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
                    onClicked: {
                        var gId = model.id !== undefined ? model.id : -1;
                        var gName = model.name !== undefined ? model.name : ("Category " + (index + 1));
                        if (gId === -100) {
                            allChannelsOpened();
                        } else if (gId === -200) {
                            favoritesOpened();
                        } else {
                            groupOpened(gId, gName);
                        }
                    }
                }
            }
        }

        // List View for Groups
        ListView {
            id: groupList
            visible: viewMode === "List"
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            spacing: 10

            model: AppController.groupViewModel

            delegate: Rectangle {
                width: groupList.width
                height: 55
                color: "#1E1E1E"
                radius: 8
                border.color: "#333333"
                border.width: 1

                RowLayout {
                    anchors.fill: parent
                    anchors.margins: 12
                    spacing: 12

                    FolderIcon {
                        visible: model.id !== -200
                        size: 32
                    }

                    Rectangle {
                        visible: model.id === -200
                        width: 32; height: 32
                        radius: 6
                        color: "#2A2A2A"
                        Text { anchors.centerIn: parent; text: "★"; color: "#FFD54F"; font.pixelSize: 18 }
                    }

                    Text {
                        text: model.name ? model.name : ("Category " + (index + 1))
                        color: "#FFFFFF"
                        font.pixelSize: 15
                        font.bold: true
                        Layout.fillWidth: true
                    }

                    Text {
                        text: (model.channelCount !== undefined ? model.channelCount : 0) + " Channels"
                        color: "#AAAAAA"
                        font.pixelSize: 12
                    }
                }

                MouseArea {
                    anchors.fill: parent
                    hoverEnabled: true
                    onEntered: parent.border.color = "#FFD54F"
                    onExited: parent.border.color = "#333333"
                    onClicked: {
                        var gId = model.id !== undefined ? model.id : -1;
                        var gName = model.name !== undefined ? model.name : ("Category " + (index + 1));
                        if (gId === -100) {
                            allChannelsOpened();
                        } else if (gId === -200) {
                            favoritesOpened();
                        } else {
                            groupOpened(gId, gName);
                        }
                    }
                }
            }
        }
    }
}
