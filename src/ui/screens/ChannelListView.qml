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

    property int groupId: -1
    property int playlistId: -1
    property string groupName: "Channels"
    property string viewMode: "List" // "List", "Grid", "Title", "Poster"
    property bool favoritesMode: false

    Settings {
        category: "ChannelView"
        property alias viewMode: root.viewMode
    }

    signal backRequested()
    signal channelOpened(string streamUrl, string channelName, string referer, string userAgent)

    function focusHeader() {
        backBtn.forceActiveFocus();
    }

    function focusMain() {
        if (viewMode === "Grid" || viewMode === "Poster") {
            if (channelGrid.currentIndex < 0 && channelGrid.count > 0) channelGrid.currentIndex = 0;
            channelGrid.forceActiveFocus();
        } else {
            if (channelList.currentIndex < 0 && channelList.count > 0) channelList.currentIndex = 0;
            channelList.forceActiveFocus();
        }
    }

    Keys.onEscapePressed: root.backRequested()

    StackView.onActivated: focusMain()

    onGroupIdChanged: {
        if (!favoritesMode && groupId !== -1) {
            AppController.channelViewModel.loadChannels(groupId);
        }
    }

    onPlaylistIdChanged: {
        if (!favoritesMode && groupId === -1 && playlistId !== -1) {
            AppController.channelViewModel.loadAllChannels(playlistId);
        }
    }

    onFavoritesModeChanged: {
        if (favoritesMode) {
            AppController.channelViewModel.loadFavorites();
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 20
        spacing: 20

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
                text: groupName
                color: "#FFFFFF"
                font.pixelSize: 24
                font.bold: true
                Layout.leftMargin: 15
            }

            Item { Layout.fillWidth: true }

            ViewModeToggle {
                id: viewToggle
                modes: ["List", "Grid", "Title", "Poster"]
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
                onSortSelected: (mode) => AppController.channelViewModel.setSortMode(mode)
            }

            TextField {
                id: searchField
                placeholderText: "Search channel..."
                color: "#FFFFFF"
                focus: true
                KeyNavigation.left: sortMenu
                background: Rectangle { 
                    color: "#1E1E1E"
                    radius: 6
                    implicitHeight: 40
                    implicitWidth: 200
                    border.color: searchField.activeFocus ? "#FFD54F" : "#333333"
                    border.width: searchField.activeFocus ? 2 : 1
                }
                leftPadding: 10
                onTextChanged: AppController.channelViewModel.setSearchQuery(text)
            }
        }

        ListView {
            id: channelList
            visible: viewMode === "List" || viewMode === "Title"
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            spacing: viewMode === "Title" ? 4 : 10
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

            model: AppController.channelViewModel

            delegate: Rectangle {
                width: channelList.width
                height: viewMode === "Title" ? 36 : 60
                color: "#1E1E1E"
                radius: 8
                property bool isActiveItem: channelList.activeFocus && ListView.isCurrentItem
                border.color: isActiveItem ? "#FFD54F" : "#333333"
                border.width: isActiveItem ? 2 : 1

                RowLayout {
                    anchors.fill: parent
                    anchors.margins: viewMode === "Title" ? 6 : 10
                    spacing: 15

                    ChannelLogo {
                        visible: viewMode === "List"
                        width: 60; height: 40
                        source: model.logoUrl ? model.logoUrl : ""
                        fallbackText: "LOGO"
                    }

                    Text {
                        id: titleText
                        text: model.name ? model.name : ("Channel " + (index + 1))
                        color: "#FFFFFF"
                        font.pixelSize: viewMode === "Title" ? 13 : 16
                        font.bold: viewMode !== "Title"
                        elide: Text.ElideRight
                        maximumLineCount: 1
                        wrapMode: Text.NoWrap
                        clip: true
                        Layout.fillWidth: true
                        Layout.minimumWidth: 0
                        Layout.preferredWidth: 0
                        Layout.maximumWidth: parent.width

                        HoverHandler { id: titleHover }
                        ToolTip.visible: titleHover.hovered && titleText.truncated
                        ToolTip.text: model.name ? model.name : ""
                        ToolTip.delay: 400
                    }

                    Button {
                        visible: viewMode === "List"
                        text: "★"
                        background: Rectangle { color: "transparent" }
                        contentItem: Text { text: parent.text; color: model.isFavorite ? "#FFD54F" : "#555555"; font.pixelSize: 20 }
                        onClicked: AppController.channelViewModel.toggleFavorite(model.id)
                    }

                    Button {
                        text: "Play"
                        background: Rectangle { color: "#FFD54F"; radius: 6; implicitHeight: viewMode === "Title" ? 24 : 35; implicitWidth: viewMode === "Title" ? 60 : 80 }
                        contentItem: Text { text: parent.text; color: "#121212"; font.bold: true; font.pixelSize: viewMode === "Title" ? 11 : 13; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                        onClicked: channelOpened(model.streamUrl, model.name, model.referer, model.userAgent)
                    }
                }

                MouseArea {
                    anchors.fill: parent
                    hoverEnabled: true
                    z: -1
                    onEntered: parent.border.color = "#FFD54F"
                    onExited: parent.border.color = (channelList.activeFocus && ListView.isCurrentItem) ? "#FFD54F" : "#333333"
                    onClicked: {
                        channelList.currentIndex = index;
                        channelOpened(model.streamUrl, model.name, model.referer, model.userAgent);
                    }
                }

                Keys.onPressed: (event) => {
                    if (event.key === Qt.Key_Enter || event.key === Qt.Key_Return || event.key === Qt.Key_Space) {
                        channelOpened(model.streamUrl, model.name, model.referer, model.userAgent);
                        event.accepted = true;
                    } else if (event.key === Qt.Key_F) {
                        AppController.channelViewModel.toggleFavorite(model.id);
                        event.accepted = true;
                    }
                }
            }
        }

        GridView {
            id: channelGrid
            visible: viewMode === "Grid" || viewMode === "Poster"
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            focus: visible
            KeyNavigation.up: backBtn

            cellWidth: viewMode === "Poster" ? Math.floor(width / 5) : Math.floor(width / 3)
            cellHeight: viewMode === "Poster" ? 235 : 145

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

            model: AppController.channelViewModel

            delegate: Item {
                width: channelGrid.cellWidth
                height: channelGrid.cellHeight

                Keys.onPressed: (event) => {
                    if (event.key === Qt.Key_Enter || event.key === Qt.Key_Return || event.key === Qt.Key_Space) {
                        channelOpened(model.streamUrl, model.name, model.referer, model.userAgent);
                        event.accepted = true;
                    } else if (event.key === Qt.Key_F) {
                        AppController.channelViewModel.toggleFavorite(model.id);
                        event.accepted = true;
                    }
                }

                Rectangle {
                    anchors.centerIn: parent
                    width: parent.width - 15
                    height: parent.height - 15
                    color: "#1E1E1E"
                    radius: 10
                    property bool isActiveItem: channelGrid.activeFocus && parent.GridView.isCurrentItem
                    border.color: (hoverArea.containsMouse || isActiveItem) ? "#FFD54F" : "#333333"
                    border.width: isActiveItem ? 2 : 1

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 10
                        spacing: 6

                        ChannelLogo {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            source: model.logoUrl ? model.logoUrl : ""
                            fallbackText: viewMode === "Poster" ? "POSTER" : "LOGO"
                        }

                        Text {
                            id: gridTitleText
                            text: model.name ? model.name : ("Channel " + (index + 1))
                            color: "#FFFFFF"
                            font.pixelSize: 13
                            font.bold: true
                            elide: Text.ElideRight
                            maximumLineCount: 1
                            wrapMode: Text.NoWrap
                            clip: true
                            Layout.fillWidth: true
                            Layout.minimumWidth: 0
                            Layout.preferredWidth: 0
                            horizontalAlignment: Text.AlignHCenter

                            HoverHandler { id: gridTitleHover }
                            ToolTip.visible: gridTitleHover.hovered && gridTitleText.truncated
                            ToolTip.text: model.name ? model.name : ""
                            ToolTip.delay: 400
                        }
                    }

                    MouseArea {
                        id: hoverArea
                        anchors.fill: parent
                        hoverEnabled: true
                        z: -1
                        onClicked: {
                            channelGrid.currentIndex = index;
                            channelOpened(model.streamUrl, model.name, model.referer, model.userAgent);
                        }
                    }

                    Button {
                        anchors.top: parent.top
                        anchors.right: parent.right
                        anchors.margins: 4
                        width: 26; height: 26
                        text: "★"
                        background: Rectangle { color: "#80000000"; radius: 13 }
                        contentItem: Text { text: parent.text; color: model.isFavorite ? "#FFD54F" : "#AAAAAA"; font.pixelSize: 14; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                        onClicked: AppController.channelViewModel.toggleFavorite(model.id)
                    }
                }
            }
        }
    }
}