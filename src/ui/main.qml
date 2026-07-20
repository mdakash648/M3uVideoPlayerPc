import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import M3uVideoPlayer
import "components"
import "screens"

ApplicationWindow {
    id: mainWindow
    width: 1280
    height: 720
    minimumWidth: 850
    minimumHeight: 450
    visible: true
    title: qsTr("M3U Video Player Desktop")
    color: "#121212"

    property bool isPlayerActive: stackView.currentItem && stackView.currentItem.objectName === "playerView"

    // Focus Navigation: 0 = Main, 1 = Header, 2 = Sidebar
    property int currentFocusSection: 0

    // Detached player windows (VLC-style multi-window). Keeping the refs here
    // stops the JS GC from collecting a still-visible Window; main.qml itself
    // stays alive when the main window is closed (closing only hides it), so
    // detached windows survive until they are closed themselves. Qt quits the
    // app automatically once the last visible window is closed.
    property var detachedWindows: []

    // Creates one independent PlayerWindow from a full property map
    // (streamUrl/streamTitle/channelModel/...). Windows are unlimited and
    // fully separate: each has its own mpv instance, channel model and
    // fullscreen/always-on-top state, and closing one never affects another.
    function openDetachedWindow(props) {
        var index = detachedWindows.length
        var win = playerWindowComponent.createObject(null, props)
        if (!win) {
            console.warn("Failed to create detached player window:",
                         playerWindowComponent.errorString())
            return null
        }
        // Cascade like VLC so stacked windows stay grabbable
        win.x = mainWindow.x + 48 + (index % 8) * 32
        win.y = mainWindow.y + 48 + (index % 8) * 32
        detachedWindows.push(win)
        win.windowClosed.connect(function() {
            var i = mainWindow.detachedWindows.indexOf(win)
            if (i >= 0) mainWindow.detachedWindows.splice(i, 1)
        })
        win.show()
        win.raise()
        win.requestActivate()
        return win
    }

    // Local file: always a separate window (never mixed into a window that is
    // already playing). The main window's player is paused so its audio can't
    // bleed under the new video.
    function openDetachedPlayer(filePath, name) {
        if (isPlayerActive && stackView.currentItem && typeof stackView.currentItem.pause === "function") {
            stackView.currentItem.pause();
        }
        // Own folder model per window: Next/Previous/playlist panel navigate
        // this file's folder without clobbering the shared channelViewModel.
        var model = AppController.createLocalChannelModel(filePath)
        openDetachedWindow({
            "streamUrl": filePath,
            "streamTitle": name,
            "channelModel": model,
            "channelType": 1,  // MOVIE: VOD seek behavior + folder auto-next
            "playlistId": 0,   // local files never write history/resume rows
            "groupId": 0
        })
    }

    // Database channel in its own window (middle-click on a channel). Keeps
    // the channel's real playlist/group context so resume and the in-player
    // playlist panel work exactly like in the main window.
    function openChannelInNewWindow(info, resumePositionMs) {
        var model = AppController.createDetachedChannelModel(
            info.groupId !== undefined ? info.groupId : 0,
            info.playlistId !== undefined ? info.playlistId : 0)
        openDetachedWindow({
            "streamUrl": info.streamUrl,
            "streamTitle": info.channelName,
            "streamReferer": info.referer !== undefined ? info.referer : "",
            "streamUserAgent": info.userAgent !== undefined ? info.userAgent : "",
            "channelType": info.channelType !== undefined ? info.channelType : 3,
            "playlistId": info.playlistId !== undefined ? info.playlistId : 0,
            "groupId": info.groupId !== undefined ? info.groupId : 0,
            "groupTitle": info.groupTitle !== undefined ? info.groupTitle : "",
            "resumePositionMs": resumePositionMs !== undefined ? resumePositionMs : 0,
            "channelModel": model
        })
    }

    Component {
        id: playerWindowComponent
        PlayerWindow {}
    }

    onClosing: {
        // With detached windows open the app keeps running and this window
        // only hides — pop a playing PlayerView so it can't keep playing
        // invisibly (Component.onDestruction also saves its resume position).
        if (isPlayerActive) doPop()
    }

    property var forwardHistory: []
    property bool isNavigatingForward: false

    function clearForwardHistory() {
        if (!isNavigatingForward) {
            forwardHistory = [];
        }
    }

    function doPush(comp, props) {
        clearForwardHistory();
        stackView.push(comp, props !== undefined ? props : {});
    }

    function doReplace(comp, props) {
        clearForwardHistory();
        stackView.replace(comp, props !== undefined ? props : {});
    }

    function saveItemState(item) {
        if (!item) return null;
        if (item instanceof PlaylistListView) return { type: "PlaylistListView", props: {} };
        if (item instanceof GroupListView) return { type: "GroupListView", props: { playlistId: item.playlistId, playlistName: item.playlistName } };
        if (item instanceof ChannelListView) return { type: "ChannelListView", props: { groupId: item.groupId, groupName: item.groupName, playlistId: item.playlistId, deletable: item.deletable, favoritesMode: item.favoritesMode } };
        if (item && item.objectName === "playerView") return { type: "PlayerView", props: { streamUrl: item.streamUrl, streamTitle: item.streamTitle, streamReferer: item.streamReferer, streamUserAgent: item.streamUserAgent, channelType: item.channelType, playlistId: item.playlistId, groupId: item.groupId, groupTitle: item.groupTitle, resumePositionMs: item.resumePositionMs } };
        if (item instanceof SettingsView) return { type: "SettingsView", props: {} };
        if (item instanceof SettingsPlaylistsView) return { type: "SettingsPlaylistsView", props: {} };
        if (item instanceof DirectLinkView) return { type: "DirectLinkView", props: {} };
        return null;
    }

    function doPop() {
        if (stackView.depth > 1) {
            var state = saveItemState(stackView.currentItem);
            if (state) {
                var newHistory = forwardHistory;
                newHistory.push(state);
                forwardHistory = newHistory;
            }
            stackView.pop();
        }
    }

    function handleForward() {
        if (forwardHistory.length > 0) {
            var newHistory = forwardHistory;
            var state = newHistory.pop();
            forwardHistory = newHistory;
            isNavigatingForward = true;
            
            if (state.type === "PlaylistListView") stackView.push(playlistViewComponent, state.props);
            else if (state.type === "GroupListView") stackView.push(groupViewComponent, state.props);
            else if (state.type === "ChannelListView") stackView.push(channelViewComponent, state.props);
            else if (state.type === "PlayerView") stackView.push(playerViewComponent, state.props);
            else if (state.type === "SettingsView") stackView.push(settingsViewComponent, state.props);
            else if (state.type === "SettingsPlaylistsView") stackView.push(settingsPlaylistsComponent, state.props);
            else if (state.type === "DirectLinkView") stackView.push(directLinkViewComponent, state.props);
            
            isNavigatingForward = false;
        }
    }

    function applyFocusSection() {
        if (currentFocusSection === 0) {
            if (stackView.currentItem && stackView.currentItem.focusMain) {
                stackView.currentItem.focusMain();
            }
        } else if (currentFocusSection === 1) {
            if (stackView.currentItem && stackView.currentItem.focusHeader) {
                stackView.currentItem.focusHeader();
            }
        } else if (currentFocusSection === 2) {
            if (!isPlayerActive) {
                sidebar.focusFirstItem();
            } else {
                // If player is active, skip sidebar and go back to main
                currentFocusSection = 0;
                applyFocusSection();
            }
        }
    }

    function handleBack() {
        if (stackView.currentItem && stackView.currentItem.objectName === "playerView") {
            var player = stackView.currentItem;
            if (player.isFullscreen) {
                player.toggleFullscreen();
            } else if (player.playlistOpen) {
                player.playlistOpen = false;
            } else {
                doPop();
            }
        } else {
            doPop();
        }
    }

    MouseArea {
        anchors.fill: parent
        acceptedButtons: Qt.BackButton | Qt.ForwardButton
        z: 9999
        onClicked: (mouse) => {
            if (mouse.button === Qt.BackButton) {
                mainWindow.handleBack();
                mouse.accepted = true;
            } else if (mouse.button === Qt.ForwardButton) {
                mainWindow.handleForward();
                mouse.accepted = true;
            }
        }
    }

    Shortcut {
        sequence: "Tab"
        onActivated: {
            currentFocusSection = (currentFocusSection + 1) % 3;
            applyFocusSection();
        }
    }

    Shortcut {
        sequence: "Shift+Tab"
        onActivated: {
            currentFocusSection = (currentFocusSection - 1 + 3) % 3;
            applyFocusSection();
        }
    }

    Item {
        focus: true
        Keys.onPressed: (event) => {
             // global fallback to prevent losing focus completely
        }
    }

    Component.onCompleted: {
        currentFocusSection = 0;
        applyFocusSection();
        
        if (!AppController.getAndSetDemoPromptShown()) {
            demoDataDialog.open();
        }
    }

    RowLayout {
        anchors.fill: parent
        spacing: 0

        // Sidebar Navigation
        Sidebar {
            id: sidebar
            Layout.fillHeight: true
            Layout.preferredWidth: mainWindow.isPlayerActive ? 0 : width
            visible: !mainWindow.isPlayerActive
            
            onNavigationRequested: (page) => {
                if (page === "Playlists") {
                    doReplace(playlistViewComponent)
                } else if (page === "DirectLink") {
                    doReplace(directLinkViewComponent)
                } else if (page === "Settings") {
                    doReplace(settingsViewComponent)
                }
                // Add more pages here as they are created
            }
        }

        // Main Content Area
        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: "#121212"
            
            StackView {
                id: stackView
                anchors.fill: parent
                initialItem: playlistViewComponent
                
                pushEnter: Transition {
                    PropertyAnimation {
                        property: "opacity"
                        from: 0
                        to: 1
                        duration: 200
                    }
                }
                pushExit: Transition {
                    PropertyAnimation {
                        property: "opacity"
                        from: 1
                        to: 0
                        duration: 200
                    }
                }
                replaceEnter: Transition {
                    PropertyAnimation {
                        property: "opacity"
                        from: 0
                        to: 1
                        duration: 200
                    }
                }
                replaceExit: Transition {
                    PropertyAnimation {
                        property: "opacity"
                        from: 1
                        to: 0
                        duration: 200
                    }
                }
                popEnter: Transition {
                    PropertyAnimation {
                        property: "opacity"
                        from: 0
                        to: 1
                        duration: 200
                    }
                }
                popExit: Transition {
                    PropertyAnimation {
                        property: "opacity"
                        from: 1
                        to: 0
                        duration: 200
                    }
                }
            }
        }
    }

    // Components for StackView
    Component {
        id: playlistViewComponent
        PlaylistListView {
            onPlaylistOpened: function(id, name) {
                doPush(groupViewComponent, { "playlistId": id, "playlistName": name })
            }
        }
    }

    Component {
        id: groupViewComponent
        GroupListView {
            onBackRequested: doPop()
            onGroupOpened: function(id, name) {
                // Movies/channels inside the History playlist can be deleted
                doPush(channelViewComponent, { "groupId": id, "groupName": name, "deletable": playlistName === "History" })
            }
            onAllChannelsOpened: {
                doPush(channelViewComponent, { "groupId": -1, "groupName": "All Channels", "playlistId": playlistId, "deletable": playlistName === "History" })
            }
            onFavoritesOpened: {
                doPush(channelViewComponent, { "groupId": -1, "groupName": "Favorites", "favoritesMode": true })
            }
            onResumeRequested: (resume) => mainWindow.resumeLastPlayed(resume)
        }
    }

    Component {
        id: channelViewComponent
        ChannelListView {
            onBackRequested: doPop()
            onChannelOpened: function(streamUrl, channelName, referer, userAgent, channelType, channelPlaylistId, channelGroupId) {
                mainWindow.openChannel({
                    "streamUrl": streamUrl,
                    "channelName": channelName,
                    "referer": referer !== undefined ? referer : "",
                    "userAgent": userAgent !== undefined ? userAgent : "",
                    "channelType": channelType !== undefined ? channelType : 3,
                    "playlistId": channelPlaylistId !== undefined ? channelPlaylistId : 0,
                    "groupId": channelGroupId !== undefined ? channelGroupId : 0,
                    "groupTitle": groupName
                })
            }
            onChannelOpenedNewWindow: function(streamUrl, channelName, referer, userAgent, channelType, channelPlaylistId, channelGroupId) {
                mainWindow.openChannel({
                    "streamUrl": streamUrl,
                    "channelName": channelName,
                    "referer": referer !== undefined ? referer : "",
                    "userAgent": userAgent !== undefined ? userAgent : "",
                    "channelType": channelType !== undefined ? channelType : 3,
                    "playlistId": channelPlaylistId !== undefined ? channelPlaylistId : 0,
                    "groupId": channelGroupId !== undefined ? channelGroupId : 0,
                    "groupTitle": groupName,
                    "newWindow": true
                })
            }
            onResumeRequested: (resume) => mainWindow.resumeLastPlayed(resume)
        }
    }

    // ===== Playback resume =====
    // Single choke point for opening a channel from any list/grid/key press.
    // Movies/series with a saved position (>30s, <95%) ask Continue/StartOver;
    // everything else (live TV, unwatched, direct links) plays immediately.
    // info.newWindow === true routes playback into a separate window
    // (middle-click) instead of the embedded player.
    function openChannel(info) {
        var isVod = info.channelType === 1 || info.channelType === 2; // MOVIE || SERIES
        if (isVod) {
            var resume = AppController.getMovieResume(info.streamUrl);
            var nearEnd = resume.durationMs > 0 && resume.positionMs / resume.durationMs >= 0.95;
            if (resume.found && resume.positionMs > 30000 && !nearEnd) {
                resumeDialog.askFor(info, resume.positionMs, resume.durationMs);
                return;
            }
        }
        mainWindow.pushPlayer(info, 0);
    }

    function pushPlayer(info, resumePositionMs) {
        if (info.newWindow === true) {
            openChannelInNewWindow(info, resumePositionMs)
            return
        }
        doPush(playerViewComponent, {
            "streamUrl": info.streamUrl,
            "streamTitle": info.channelName,
            "streamReferer": info.referer,
            "streamUserAgent": info.userAgent,
            "channelType": info.channelType,
            "playlistId": info.playlistId,
            "groupId": info.groupId,
            "groupTitle": info.groupTitle !== undefined ? info.groupTitle : "",
            "resumePositionMs": resumePositionMs
        })
    }

    // Floating play button (MX-Player style): open the playlist's last-played
    // video directly — movies at their saved position, live TV from the start.
    // Loads the remembered folder into the shared channel model first so the
    // player's Next/Previous buttons navigate within that folder.
    function resumeLastPlayed(resume) {
        if (resume.groupId > 0) {
            AppController.channelViewModel.loadChannels(resume.groupId)
        } else {
            AppController.channelViewModel.loadAllChannels(resume.playlistId)
        }
        var isVod = resume.contentType === 1 || resume.contentType === 2; // MOVIE || SERIES
        mainWindow.pushPlayer({
            "streamUrl": resume.streamUrl,
            "channelName": resume.title,
            "referer": resume.referer,
            "userAgent": resume.userAgent,
            "channelType": resume.contentType,
            "playlistId": resume.playlistId,
            "groupId": resume.groupId,
            "groupTitle": resume.groupTitle
        }, isVod ? resume.positionMs : 0)
    }

    ResumeDialog {
        id: resumeDialog
        parent: Overlay.overlay
        onContinueChosen: function(payload) {
            mainWindow.pushPlayer(payload, resumeDialog.positionMs)
        }
        onStartOverChosen: function(payload) {
            AppController.clearMovieResume(payload.streamUrl)
            mainWindow.pushPlayer(payload, 0)
        }
    }

    Component {
        id: playerViewComponent
        PlayerView {
            onBackRequested: doPop()
        }
    }

    Component {
        id: settingsViewComponent
        SettingsView {
            onPlaylistsRequested: doPush(settingsPlaylistsComponent)
        }
    }

    Component {
        id: settingsPlaylistsComponent
        SettingsPlaylistsView {
            onBackRequested: doPop()
        }
    }

    Component {
        id: directLinkViewComponent
        DirectLinkView {
            onPlayRequested: function(streamUrl, referer, userAgent) {
                // Resolves .m3u/.m3u8 channel playlists (importing every entry
                // with its http-referrer/user-agent into "History") and
                // answers via directLinkReady below.
                AppController.openDirectLink(streamUrl, referer, userAgent)
            }
        }
    }

    // Direct link resolved: play it with the headers that came either from
    // the input fields or from the parsed M3U entry itself. If a video is
    // already playing in this window (e.g. a double-clicked .m3u arrives
    // mid-playback), the stream opens in its own detached window instead of
    // hijacking the current one — same VLC rule as local files.
    Connections {
        target: AppController
        function onDirectLinkReady(streamUrl, name, referer, userAgent) {
            var props = {
                "streamUrl": streamUrl,
                "streamTitle": name !== "" ? name : "Direct Stream",
                "streamReferer": referer,
                "streamUserAgent": userAgent
            }
            if (mainWindow.isPlayerActive) {
                props["channelType"] = AppController.channelTypeForUrl(streamUrl)
                openDetachedWindow(props)
            } else {
                doPush(playerViewComponent, props)
            }
        }
    }

    // Local video file opened (double-click / "Open with"): play it WITHOUT
    // saving to History (playlistId 0 disables progress/history writes).
    // ALWAYS opens in its own independent window (VLC-style multi-window)
    // with its own folder model — never in the main window, so it can never
    // mix with a video already playing there.
    Connections {
        target: AppController
        function onLocalVideoReady(filePath, name) {
            mainWindow.openDetachedPlayer(filePath, name)
        }
    }

    // When an .m3u/.m3u8 file is imported (file picker or double-clicked
    // file), jump to the Playlists page so the user sees it appear.
    Connections {
        target: AppController
        function onM3uFileOpened(name) {
            if (!mainWindow.isPlayerActive) {
                doReplace(playlistViewComponent)
            }
        }
    }

    Dialog {
        id: demoDataDialog
        parent: Overlay.overlay
        anchors.centerIn: parent
        width: 400
        modal: true
        focus: true
        
        background: Rectangle {
            color: "#1E1E1E"
            radius: 12
            border.color: "#333333"
        }

        contentItem: ColumnLayout {
            spacing: 16
            
            Text {
                text: qsTr("Would you like to load the demo data?")
                color: "#FFD54F"
                font.pixelSize: 18
                font.bold: true
                Layout.fillWidth: true
                wrapMode: Text.WordWrap
            }
            
            Text {
                text: qsTr("You can always load it later from Backup & Restore settings.")
                color: "#AAAAAA"
                font.pixelSize: 13
                Layout.fillWidth: true
                wrapMode: Text.WordWrap
            }

            RowLayout {
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignRight
                spacing: 12
                
                Button {
                    text: qsTr("No, thanks")
                    background: Rectangle { color: parent.hovered ? "#333333" : "#2A2A2A"; radius: 8 }
                    contentItem: Text { text: parent.text; color: "#FFFFFF"; font.pixelSize: 14; padding: 12 }
                    onClicked: demoDataDialog.close()
                }
                
                Button {
                    text: qsTr("Yes, load it")
                    background: Rectangle { color: parent.hovered ? "#FFCA28" : "#FFD54F"; radius: 8 }
                    contentItem: Text { text: parent.text; color: "#121212"; font.pixelSize: 14; font.bold: true; padding: 12 }
                    onClicked: {
                        demoDataDialog.close()
                        AppController.loadDemoData()
                    }
                }
            }
        }
    }
}
