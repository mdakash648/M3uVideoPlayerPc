import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import M3uVideoPlayer
import "../components"

// Settings → Playlists: every saved playlist with Edit / Update / Remove.
// This is the only place playlists can be deleted from.
Rectangle {
    id: root
    color: "#121212"

    signal backRequested()

    function focusHeader() { backBtn.forceActiveFocus(); }
    function focusMain() {
        if (playlistList.currentIndex < 0 && playlistList.count > 0) playlistList.currentIndex = 0;
        playlistList.forceActiveFocus();
    }

    Keys.onEscapePressed: root.backRequested()

    StackView.onActivated: {
        AppController.playlistViewModel.loadPlaylists();
        focusMain();
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 24
        spacing: 20

        RowLayout {
            Layout.fillWidth: true

            Button {
                id: backBtn
                text: "← Back"
                Keys.onReturnPressed: backBtn.clicked()
                Keys.onEnterPressed: backBtn.clicked()
                background: Rectangle {
                    color: "transparent"
                    implicitWidth: 70
                    implicitHeight: 32
                    border.color: backBtn.activeFocus ? "#FFD54F" : "transparent"
                    border.width: backBtn.activeFocus ? 1 : 0
                    radius: 4
                }
                contentItem: Text {
                    text: parent.text; color: "#FFD54F"; font.bold: true; font.pixelSize: 16
                    horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter
                }
                onClicked: root.backRequested()
            }

            Text {
                text: qsTr("Playlists")
                color: "#FFFFFF"
                font.pixelSize: 22
                font.bold: true
                Layout.leftMargin: 12
                Layout.fillWidth: true
            }
        }

        ListView {
            id: playlistList
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            spacing: 10
            KeyNavigation.up: backBtn

            model: AppController.playlistViewModel

            delegate: Rectangle {
                id: row
                width: playlistList.width
                height: 84
                radius: 10
                color: "#1E1E1E"
                property bool isActiveItem: playlistList.activeFocus && ListView.isCurrentItem
                border.color: (rowHover.containsMouse || isActiveItem) ? "#FFD54F" : "#333333"
                border.width: isActiveItem ? 2 : 1

                // History is internal: no source to edit or refresh, and it
                // shouldn't be removed from here either.
                readonly property bool isInternal: model.url !== undefined
                                                   && model.url.indexOf("internal://") === 0

                MouseArea {
                    id: rowHover
                    anchors.fill: parent
                    hoverEnabled: true
                    onClicked: playlistList.currentIndex = index
                }

                Keys.onPressed: (event) => {
                    if ((event.key === Qt.Key_Enter || event.key === Qt.Key_Return) && !row.isInternal) {
                        editDialog.openFor(model.id, model.name, model.url,
                                           model.updateFrequency, model.type,
                                           model.username, model.password);
                        event.accepted = true;
                    } else if (event.key === Qt.Key_Delete && !row.isInternal) {
                        removeConfirm.askFor(model.id, model.name);
                        event.accepted = true;
                    }
                }

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 16
                    anchors.rightMargin: 16
                    spacing: 14

                    Rectangle {
                        width: 46; height: 46
                        radius: 8
                        color: "#2A2A2A"
                        FolderIcon { anchors.centerIn: parent; size: 26 }
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 2
                        Text {
                            text: model.name ? model.name : ("Playlist " + (index + 1))
                            color: "#FFFFFF"
                            font.pixelSize: 15
                            font.bold: true
                            elide: Text.ElideRight
                            Layout.fillWidth: true
                        }
                        Text {
                            text: {
                                var kind = row.isInternal ? "Internal" : (model.type ? "Xtream Codes" : "M3U");
                                var updated = model.lastUpdated && !isNaN(model.lastUpdated.getTime())
                                              ? "Updated " + Qt.formatDateTime(model.lastUpdated, "yyyy-MM-dd hh:mm")
                                              : "Never updated";
                                return kind + " · " + updated;
                            }
                            color: "#AAAAAA"
                            font.pixelSize: 12
                            elide: Text.ElideRight
                            Layout.fillWidth: true
                        }
                    }

                    // Edit
                    Button {
                        visible: !row.isInternal
                        text: "Edit"
                        implicitHeight: 34
                        background: Rectangle {
                            color: parent.hovered ? "#333333" : "#2A2A2A"
                            radius: 6
                            border.color: "#444444"
                        }
                        contentItem: Text {
                            text: parent.text; color: "#FFD54F"; font.pixelSize: 13; font.bold: true
                            horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter
                            leftPadding: 10; rightPadding: 10
                        }
                        onClicked: editDialog.openFor(model.id, model.name, model.url,
                                                      model.updateFrequency, model.type,
                                                      model.username, model.password)
                    }

                    // Update now
                    Button {
                        visible: !row.isInternal
                        text: "⟳ Update"
                        implicitHeight: 34
                        background: Rectangle {
                            color: parent.hovered ? "#333333" : "#2A2A2A"
                            radius: 6
                            border.color: "#444444"
                        }
                        contentItem: Text {
                            text: parent.text; color: "#FFFFFF"; font.pixelSize: 13
                            horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter
                            leftPadding: 10; rightPadding: 10
                        }
                        onClicked: AppController.playlistViewModel.refreshPlaylistAsync(model.id)
                    }

                    // Remove
                    Button {
                        visible: !row.isInternal
                        text: "Remove"
                        implicitHeight: 34
                        background: Rectangle {
                            color: parent.hovered ? "#44FF0000" : "#2A2A2A"
                            radius: 6
                            border.color: parent.hovered ? "#FF6666" : "#444444"
                        }
                        contentItem: Text {
                            text: parent.text
                            color: parent.hovered ? "#FF6666" : "#AAAAAA"
                            font.pixelSize: 13
                            horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter
                            leftPadding: 10; rightPadding: 10
                        }
                        onClicked: removeConfirm.askFor(model.id, model.name)
                    }

                    // Clear History (the internal History playlist only) —
                    // wipes every saved movie/link but keeps the playlist row.
                    Button {
                        visible: row.isInternal
                        text: "Clear History"
                        implicitHeight: 34
                        background: Rectangle {
                            color: parent.hovered ? "#44FF0000" : "#2A2A2A"
                            radius: 6
                            border.color: parent.hovered ? "#FF6666" : "#444444"
                        }
                        contentItem: Text {
                            text: parent.text
                            color: parent.hovered ? "#FF6666" : "#AAAAAA"
                            font.pixelSize: 13
                            horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter
                            leftPadding: 10; rightPadding: 10
                        }
                        onClicked: clearHistoryConfirm.open()
                    }
                }
            }

            Text {
                anchors.centerIn: parent
                visible: playlistList.count === 0
                text: "No playlists saved yet"
                color: "#666666"
                font.pixelSize: 15
            }
        }
    }

    EditPlaylistDialog {
        id: editDialog
    }

    // Delete confirmation
    Popup {
        id: removeConfirm
        anchors.centerIn: parent
        width: 400
        modal: true
        focus: true
        padding: 20
        background: Rectangle { color: "#1E1E1E"; radius: 12; border.color: "#333333"; border.width: 1 }

        property int targetId: -1
        property string targetName: ""

        function askFor(id, name) {
            targetId = id;
            targetName = name;
            open();
        }

        contentItem: ColumnLayout {
            spacing: 18

            Text {
                text: "Remove Playlist"
                color: "#FF6666"
                font.pixelSize: 18
                font.bold: true
            }

            Text {
                text: "Remove \"" + removeConfirm.targetName + "\" and all its channels? This cannot be undone."
                color: "#DDDDDD"
                font.pixelSize: 13
                lineHeight: 1.2
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }

            RowLayout {
                Layout.fillWidth: true
                Layout.topMargin: 4
                spacing: 12
                Item { Layout.fillWidth: true }

                Button {
                    text: "Cancel"
                    background: Rectangle {
                        color: parent.hovered ? "#2A2A2A" : "transparent"
                        radius: 6; implicitHeight: 38; implicitWidth: 90
                        border.color: "#444444"; border.width: 1
                    }
                    contentItem: Text {
                        text: parent.text; color: parent.hovered ? "#FFFFFF" : "#AAAAAA"; font.pixelSize: 14
                        horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter
                    }
                    onClicked: removeConfirm.close()
                }

                Button {
                    text: "Remove"
                    background: Rectangle {
                        color: parent.hovered ? "#E04444" : "#CC3333"
                        radius: 6; implicitHeight: 38; implicitWidth: 110
                    }
                    contentItem: Text {
                        text: parent.text; color: "#FFFFFF"; font.bold: true; font.pixelSize: 14
                        horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter
                    }
                    onClicked: {
                        AppController.playlistViewModel.deletePlaylist(removeConfirm.targetId);
                        removeConfirm.close();
                    }
                }
            }
        }
    }

    // Clear History confirmation
    Popup {
        id: clearHistoryConfirm
        anchors.centerIn: parent
        width: 400
        modal: true
        focus: true
        padding: 20
        background: Rectangle { color: "#1E1E1E"; radius: 12; border.color: "#333333"; border.width: 1 }

        contentItem: ColumnLayout {
            spacing: 18

            Text {
                text: "Clear History"
                color: "#FF6666"
                font.pixelSize: 18
                font.bold: true
            }

            Text {
                text: "Delete every saved movie and direct link from your History? This cannot be undone."
                color: "#DDDDDD"
                font.pixelSize: 13
                lineHeight: 1.2
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }

            RowLayout {
                Layout.fillWidth: true
                Layout.topMargin: 4
                spacing: 12
                Item { Layout.fillWidth: true }

                Button {
                    text: "Cancel"
                    background: Rectangle {
                        color: parent.hovered ? "#2A2A2A" : "transparent"
                        radius: 6; implicitHeight: 38; implicitWidth: 90
                        border.color: "#444444"; border.width: 1
                    }
                    contentItem: Text {
                        text: parent.text; color: parent.hovered ? "#FFFFFF" : "#AAAAAA"; font.pixelSize: 14
                        horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter
                    }
                    onClicked: clearHistoryConfirm.close()
                }

                Button {
                    text: "Clear All"
                    background: Rectangle {
                        color: parent.hovered ? "#E04444" : "#CC3333"
                        radius: 6; implicitHeight: 38; implicitWidth: 110
                    }
                    contentItem: Text {
                        text: parent.text; color: "#FFFFFF"; font.bold: true; font.pixelSize: 14
                        horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter
                    }
                    onClicked: {
                        AppController.clearHistory();
                        clearHistoryConfirm.close();
                    }
                }
            }
        }
    }
}
