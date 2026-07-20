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
    // When true (History content), each item shows a delete button.
    property bool deletable: false

    Settings {
        category: "ChannelView"
        property alias viewMode: root.viewMode
    }

    signal backRequested()
    signal channelOpened(string streamUrl, string channelName, string referer, string userAgent,
                         int channelType, int channelPlaylistId, int channelGroupId)
    // Middle-click (mouse wheel button): open the channel in a new,
    // independent player window instead of the embedded player.
    signal channelOpenedNewWindow(string streamUrl, string channelName, string referer, string userAgent,
                                  int channelType, int channelPlaylistId, int channelGroupId)
    // Floating play button: resume the playlist's last-played video
    signal resumeRequested(var resume)

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

    StackView.onActivated: {
        focusMain();
        // root.playlistId is only set in "All Channels" mode — otherwise take
        // the playlist from the loaded channels themselves.
        var pid = root.playlistId;
        if (pid <= 0) pid = AppController.channelViewModel.channelPlaylistId(0);
        resumeFab.playlistId = pid > 0 ? pid : -1;
        resumeFab.refresh(); // a play session may have updated the resume row
    }

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
                Keys.onReturnPressed: backBtn.clicked()
                Keys.onEnterPressed: backBtn.clicked()
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
                        onClicked: channelOpened(model.streamUrl, model.name, model.referer, model.userAgent, model.type, model.playlistId, model.groupId)
                    }

                    // Delete (History only)
                    Button {
                        visible: root.deletable
                        text: "🗑"
                        implicitWidth: viewMode === "Title" ? 24 : 35
                        implicitHeight: viewMode === "Title" ? 24 : 35
                        background: Rectangle {
                            color: parent.hovered ? "#44FF0000" : "transparent"
                            radius: 6
                            border.color: parent.hovered ? "#FF6666" : "transparent"
                            border.width: parent.hovered ? 1 : 0
                        }
                        contentItem: Text {
                            text: parent.text
                            color: parent.hovered ? "#FF6666" : "#888888"
                            font.pixelSize: viewMode === "Title" ? 12 : 15
                            horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter
                        }
                        onClicked: deleteConfirm.askFor(model.id, model.name)
                    }
                }

                MouseArea {
                    anchors.fill: parent
                    hoverEnabled: true
                    acceptedButtons: Qt.LeftButton | Qt.MiddleButton
                    z: -1
                    onEntered: parent.border.color = "#FFD54F"
                    onExited: parent.border.color = (channelList.activeFocus && ListView.isCurrentItem) ? "#FFD54F" : "#333333"
                    onClicked: (mouse) => {
                        channelList.currentIndex = index;
                        if (mouse.button === Qt.MiddleButton) {
                            channelOpenedNewWindow(model.streamUrl, model.name, model.referer, model.userAgent, model.type, model.playlistId, model.groupId);
                        } else {
                            channelOpened(model.streamUrl, model.name, model.referer, model.userAgent, model.type, model.playlistId, model.groupId);
                        }
                    }
                }

                Keys.onPressed: (event) => {
                    if (event.key === Qt.Key_Enter || event.key === Qt.Key_Return || event.key === Qt.Key_Space) {
                        channelOpened(model.streamUrl, model.name, model.referer, model.userAgent, model.type, model.playlistId, model.groupId);
                        event.accepted = true;
                    } else if (event.key === Qt.Key_F) {
                        AppController.channelViewModel.toggleFavorite(model.id);
                        event.accepted = true;
                    } else if (event.key === Qt.Key_Delete && root.deletable) {
                        deleteConfirm.askFor(model.id, model.name);
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

            // Settings → Grid/Poster View Columns (0 = Auto → 4 per row)
            readonly property int gridSettingsColumns: AppController.settings.gridColumns > 0
                                                       ? AppController.settings.gridColumns : 4
            readonly property int posterSettingsColumns: AppController.settings.posterColumns > 0
                                                         ? AppController.settings.posterColumns : 4
            cellWidth: viewMode === "Poster" ? Math.floor(width / posterSettingsColumns)
                                             : Math.floor(width / gridSettingsColumns)
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
                        channelOpened(model.streamUrl, model.name, model.referer, model.userAgent, model.type, model.playlistId, model.groupId);
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
                        acceptedButtons: Qt.LeftButton | Qt.MiddleButton
                        z: -1
                        onClicked: (mouse) => {
                            channelGrid.currentIndex = index;
                            if (mouse.button === Qt.MiddleButton) {
                                channelOpenedNewWindow(model.streamUrl, model.name, model.referer, model.userAgent, model.type, model.playlistId, model.groupId);
                            } else {
                                channelOpened(model.streamUrl, model.name, model.referer, model.userAgent, model.type, model.playlistId, model.groupId);
                            }
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

                    // Delete (History only) — top-left corner
                    Button {
                        visible: root.deletable
                        anchors.top: parent.top
                        anchors.left: parent.left
                        anchors.margins: 4
                        width: 26; height: 26
                        text: "🗑"
                        background: Rectangle { color: parent.hovered ? "#44FF0000" : "#80000000"; radius: 13 }
                        contentItem: Text { text: parent.text; color: parent.hovered ? "#FF6666" : "#CCCCCC"; font.pixelSize: 13; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                        onClicked: deleteConfirm.askFor(model.id, model.name)
                    }
                }
            }
        }
    }

    // ===== Delete confirmation (History items) =====
    Popup {
        id: deleteConfirm
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

        // ColumnLayout as the contentItem so the Popup sizes to its content
        // (avoids the cramped, fixed-height look).
        contentItem: ColumnLayout {
            spacing: 18

            Text {
                text: "Delete from History"
                color: "#FF6666"
                font.pixelSize: 18
                font.bold: true
            }

            Text {
                text: "Remove \"" + deleteConfirm.targetName + "\" from your history? This cannot be undone."
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
                    onClicked: deleteConfirm.close()
                }

                Button {
                    text: "Delete"
                    background: Rectangle {
                        color: parent.hovered ? "#E04444" : "#CC3333"
                        radius: 6; implicitHeight: 38; implicitWidth: 110
                    }
                    contentItem: Text {
                        text: parent.text; color: "#FFFFFF"; font.bold: true; font.pixelSize: 14
                        horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter
                    }
                    onClicked: {
                        AppController.channelViewModel.deleteChannel(deleteConfirm.targetId);
                        deleteConfirm.close();
                    }
                }
            }
        }
    }

    // MX-Player-style floating "resume last played" button
    ResumeFab {
        id: resumeFab
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.margins: 28
        onPlayRequested: (resume) => root.resumeRequested(resume)
    }
}