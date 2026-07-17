import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtCore
import "../components"
import M3uVideoPlayer

Rectangle {
    id: root
    color: "#121212"
    focus: true

    property int playlistId: -1
    property string playlistName: "Groups / Categories"
    property string viewMode: "Grid" // "Grid" or "List"

    Settings {
        category: "GroupView"
        property alias viewMode: root.viewMode
    }

    signal backRequested()
    signal groupOpened(int groupId, string groupName)
    signal allChannelsOpened()
    signal favoritesOpened()

    function focusHeader() {
        backBtn.forceActiveFocus();
    }

    function focusMain() {
        if (viewMode === "Grid") {
            if (groupGrid.currentIndex < 0 && groupGrid.count > 0) groupGrid.currentIndex = 0;
            groupGrid.forceActiveFocus();
        } else {
            if (groupList.currentIndex < 0 && groupList.count > 0) groupList.currentIndex = 0;
            groupList.forceActiveFocus();
        }
    }

    Keys.onEscapePressed: root.backRequested()

    StackView.onActivated: focusMain()

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
                id: backBtn
                text: "← Back"
                focus: true
                KeyNavigation.right: viewToggle
                background: Rectangle { 
                    color: "transparent"
                    implicitWidth: 60
                    implicitHeight: 30
                    border.color: backBtn.activeFocus ? "#FFD54F" : "transparent"
                    border.width: backBtn.activeFocus ? 1 : 0
                    radius: 4
                }
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
                id: viewToggle
                modes: ["Grid", "List"]
                currentMode: viewMode
                focus: true
                KeyNavigation.left: backBtn
                KeyNavigation.right: sortMenu
                onModeSelected: (mode) => viewMode = mode
            }

            SortMenuButton {
                id: sortMenu
                focus: true
                KeyNavigation.left: viewToggle
                KeyNavigation.right: searchField
                onSortSelected: (mode) => AppController.groupViewModel.setSortMode(mode)
            }

            TextField {
                id: searchField
                placeholderText: "Search categories..."
                color: "#FFFFFF"
                focus: true
                KeyNavigation.left: sortMenu
                background: Rectangle { 
                    color: "#1E1E1E"
                    radius: 6
                    implicitHeight: 40
                    implicitWidth: 220
                    border.color: searchField.activeFocus ? "#FFD54F" : "#333333"
                    border.width: searchField.activeFocus ? 2 : 1
                }
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
            focus: visible
            KeyNavigation.up: backBtn

            cellWidth: Math.floor(width / 3)
            cellHeight: 140

            add: Transition {
                ParallelAnimation {
                    NumberAnimation { property: "opacity"; from: 0; to: 1; duration: 400; easing.type: Easing.OutQuart }
                    NumberAnimation { property: "scale"; from: 0.001; to: 1.0; duration: 400; easing.type: Easing.OutQuart }
                }
            }
            remove: Transition {
                ParallelAnimation {
                    NumberAnimation { property: "opacity"; to: 0; duration: 400; easing.type: Easing.OutQuart }
                    NumberAnimation { property: "scale"; to: 0.001; duration: 400; easing.type: Easing.OutQuart }
                }
            }
            displaced: Transition {
                NumberAnimation { properties: "x,y"; duration: 400; easing.type: Easing.OutQuart }
            }
            populate: Transition {
                ParallelAnimation {
                    NumberAnimation { property: "opacity"; from: 0; to: 1; duration: 400; easing.type: Easing.OutQuart }
                    NumberAnimation { property: "scale"; from: 0.001; to: 1.0; duration: 400; easing.type: Easing.OutQuart }
                }
            }

            model: AppController.groupViewModel

            delegate: Item {
                width: groupGrid.cellWidth
                height: groupGrid.cellHeight

                Keys.onPressed: (event) => {
                    if (event.key === Qt.Key_Enter || event.key === Qt.Key_Return || event.key === Qt.Key_Space) {
                        var gId = model.id !== undefined ? model.id : -1;
                        var gName = model.name !== undefined ? model.name : ("Category " + (index + 1));
                        if (gId === -100) {
                            allChannelsOpened();
                        } else if (gId === -200) {
                            favoritesOpened();
                        } else {
                            groupOpened(gId, gName);
                        }
                        event.accepted = true;
                    }
                }

                Rectangle {
                    anchors.centerIn: parent
                    width: parent.width - 15
                    height: parent.height - 15
                    color: "#1E1E1E"
                    radius: 12
                    property bool isActiveItem: groupGrid.activeFocus && parent.GridView.isCurrentItem
                    border.color: (hoverArea.containsMouse || isActiveItem) ? "#FFD54F" : "#333333"
                    border.width: isActiveItem ? 2 : 1

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
                        id: hoverArea
                        anchors.fill: parent
                        hoverEnabled: true
                        onClicked: {
                            groupGrid.currentIndex = index;
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

        // List View for Groups
        ListView {
            id: groupList
            visible: viewMode === "List"
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            spacing: 10
            focus: visible
            KeyNavigation.up: backBtn

            add: Transition {
                ParallelAnimation {
                    NumberAnimation { property: "opacity"; from: 0; to: 1; duration: 400; easing.type: Easing.OutQuart }
                    NumberAnimation { property: "scale"; from: 0.001; to: 1.0; duration: 400; easing.type: Easing.OutQuart }
                }
            }
            remove: Transition {
                ParallelAnimation {
                    NumberAnimation { property: "opacity"; to: 0; duration: 400; easing.type: Easing.OutQuart }
                    NumberAnimation { property: "scale"; to: 0.001; duration: 400; easing.type: Easing.OutQuart }
                }
            }
            displaced: Transition {
                NumberAnimation { properties: "x,y"; duration: 400; easing.type: Easing.OutQuart }
            }
            populate: Transition {
                ParallelAnimation {
                    NumberAnimation { property: "opacity"; from: 0; to: 1; duration: 400; easing.type: Easing.OutQuart }
                    NumberAnimation { property: "scale"; from: 0.001; to: 1.0; duration: 400; easing.type: Easing.OutQuart }
                }
            }

            model: AppController.groupViewModel

            delegate: Rectangle {
                width: groupList.width
                height: 55
                color: "#1E1E1E"
                radius: 8
                property bool isActiveItem: groupList.activeFocus && ListView.isCurrentItem
                border.color: isActiveItem ? "#FFD54F" : "#333333"
                border.width: isActiveItem ? 2 : 1

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
                    onExited: parent.border.color = (groupList.activeFocus && ListView.isCurrentItem) ? "#FFD54F" : "#333333"
                    onClicked: {
                        groupList.currentIndex = index;
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

                Keys.onPressed: (event) => {
                    if (event.key === Qt.Key_Enter || event.key === Qt.Key_Return || event.key === Qt.Key_Space) {
                        var gId = model.id !== undefined ? model.id : -1;
                        var gName = model.name !== undefined ? model.name : ("Category " + (index + 1));
                        if (gId === -100) {
                            allChannelsOpened();
                        } else if (gId === -200) {
                            favoritesOpened();
                        } else {
                            groupOpened(gId, gName);
                        }
                        event.accepted = true;
                    }
                }
            }
        }
    }
}
