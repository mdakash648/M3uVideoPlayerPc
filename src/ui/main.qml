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

    property bool isPlayerActive: stackView.currentItem instanceof PlayerView
    
    // Focus Navigation: 0 = Main, 1 = Header, 2 = Sidebar
    property int currentFocusSection: 0

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
                    stackView.replace(playlistViewComponent)
                } else if (page === "DirectLink") {
                    stackView.replace(directLinkViewComponent)
                } else if (page === "Settings") {
                    stackView.replace(settingsViewComponent)
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
                stackView.push(groupViewComponent, { "playlistId": id, "playlistName": name })
            }
        }
    }

    Component {
        id: groupViewComponent
        GroupListView {
            onBackRequested: stackView.pop()
            onGroupOpened: function(id, name) {
                // Movies/channels inside the History playlist can be deleted
                stackView.push(channelViewComponent, { "groupId": id, "groupName": name, "deletable": playlistName === "History" })
            }
            onAllChannelsOpened: {
                stackView.push(channelViewComponent, { "groupId": -1, "groupName": "All Channels", "playlistId": playlistId, "deletable": playlistName === "History" })
            }
            onFavoritesOpened: {
                stackView.push(channelViewComponent, { "groupId": -1, "groupName": "Favorites", "favoritesMode": true })
            }
            onResumeRequested: (resume) => mainWindow.resumeLastPlayed(resume)
        }
    }

    Component {
        id: channelViewComponent
        ChannelListView {
            onBackRequested: stackView.pop()
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
            onResumeRequested: (resume) => mainWindow.resumeLastPlayed(resume)
        }
    }

    // ===== Playback resume =====
    // Single choke point for opening a channel from any list/grid/key press.
    // Movies/series with a saved position (>30s, <95%) ask Continue/StartOver;
    // everything else (live TV, unwatched, direct links) plays immediately.
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
        stackView.push(playerViewComponent, {
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
            onBackRequested: stackView.pop()
        }
    }

    Component {
        id: settingsViewComponent
        SettingsView {
            onPlaylistsRequested: stackView.push(settingsPlaylistsComponent)
        }
    }

    Component {
        id: settingsPlaylistsComponent
        SettingsPlaylistsView {
            onBackRequested: stackView.pop()
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
    // the input fields or from the parsed M3U entry itself.
    Connections {
        target: AppController
        function onDirectLinkReady(streamUrl, name, referer, userAgent) {
            stackView.push(playerViewComponent, {
                "streamUrl": streamUrl,
                "streamTitle": name !== "" ? name : "Direct Stream",
                "streamReferer": referer,
                "streamUserAgent": userAgent
            })
        }
    }

    // When an .m3u/.m3u8 file is imported (file picker or double-clicked
    // file), jump to the Playlists page so the user sees it appear.
    Connections {
        target: AppController
        function onM3uFileOpened(name) {
            if (!mainWindow.isPlayerActive) {
                stackView.replace(playlistViewComponent)
            }
        }
    }
}
