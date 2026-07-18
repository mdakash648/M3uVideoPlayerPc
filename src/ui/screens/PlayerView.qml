import QtQuick
import QtQuick.Window
import QtQuick.Controls.Basic
import QtQuick.Layouts
import M3uVideoPlayer

Item {
    id: root
    
    property string streamUrl: ""
    property string streamTitle: "" // Allows dynamic titles
    property string streamReferer: ""
    property string streamUserAgent: ""
    property bool playlistOpen: false
    signal backRequested()

    // ===== Playback resume =====
    // Context pushed by the opener; playlistId <= 0 disables progress saving
    // (e.g. direct links). channelType gates live-vs-VOD behavior.
    property real resumePositionMs: 0
    property int channelType: 3        // Domain::ContentType (LIVE=0/MOVIE=1/SERIES=2/UNKNOWN=3)
    property int playlistId: 0
    property int groupId: 0
    property string groupTitle: ""
    // The channel's own URL — quality switching rewrites streamUrl, but
    // resume rows must stay keyed by the original playlist URL.
    property string canonicalUrl: ""

    // Keyboard control state
    property int seekStep: 10          // Current seek increment (accelerates on hold)
    property bool seekingLeft: false
    property bool seekingRight: false
    property int savedVolume: 100      // For mute toggle
    property int prevVisibility: Window.Windowed
    property real prevX: 0
    property real prevY: 0
    property real prevWidth: 0
    property real prevHeight: 0
    property bool isFullscreen: false

    // ===== Always on top (VLC-style) =====
    property bool alwaysOnTop: false

    function toggleAlwaysOnTop() {
        root.alwaysOnTop = !root.alwaysOnTop;
        AppController.setAlwaysOnTop(root.alwaysOnTop);
        osdOverlay.show(root.alwaysOnTop ? "📌 Always on Top: ON" : "📌 Always on Top: OFF");
    }

    // ===== Playback speed =====
    property real playbackSpeed: 1.0       // Persistent user-selected speed
    property bool spaceHoldTriggered: false // True once hold-to-2x has kicked in

    function setPlaybackSpeed(value) {
        root.playbackSpeed = value;
        videoPlayer.command(["set", "speed", value.toString()]);
        osdOverlay.show(value + "x Speed");
    }

    // ===== Quality switching =====
    // Detects a "1080P/720p/480P" token in the URL and rewrites it to switch
    // quality. triedUrls tracks what's been attempted for auto-fallback.
    // Local files are excluded: "Movie 1080p.mkv" is a filename, not a
    // quality variant — rewriting the path would just point at nothing.
    property var triedUrls: []

    function isLocalFile(url) {
        return url !== "" && !/^https?:\/\//i.test(url);
    }

    function detectQuality(url) {
        if (isLocalFile(url)) return "";
        var m = url.match(/(1080|720|480)[pP]/);
        return m ? m[1] : "";
    }

    function urlForQuality(url, q) {
        return url.replace(/(1080|720|480)([pP])/, q + "$2");
    }

    function startStream(url) {
        root.streamUrl = url;
        videoPlayer.mediaUrl = url;
        videoPlayer.playing = true;
    }

    // ===== Resume progress saving =====
    // Live TV: only the channel is remembered (positionMs stays 0).
    // Movies/series: position saved every 5s + on close/switch; cleared and
    // auto-advanced once playback passes ~95% or hits the end of file.
    property bool isVod: (root.channelType === 1 || root.channelType === 2) // MOVIE || SERIES

    function saveResumeProgress() {
        if (root.playlistId <= 0 || root.canonicalUrl === "") return;
        var posMs = videoPlayer.position * 1000;
        var durMs = videoPlayer.duration * 1000;
        if (root.isVod && durMs > 0 && posMs / durMs >= 0.95) {
            // Watched to the end — next open starts fresh
            AppController.clearMovieResume(root.canonicalUrl);
            AppController.saveProgress(root.playlistId, root.groupId, root.groupTitle,
                                       root.canonicalUrl, root.streamTitle,
                                       root.streamReferer, root.streamUserAgent,
                                       root.channelType, 0, durMs);
            return;
        }
        AppController.saveProgress(root.playlistId, root.groupId, root.groupTitle,
                                   root.canonicalUrl, root.streamTitle,
                                   root.streamReferer, root.streamUserAgent,
                                   root.channelType, posMs, durMs);
    }

    Timer {
        id: progressSaveTimer
        interval: 5000
        repeat: true
        running: videoPlayer.playing && root.playlistId > 0
        onTriggered: root.saveResumeProgress()
    }

    Connections {
        target: videoPlayer
        function onEndReached() {
            if (!root.isVod) return; // live streams ending is a stall, not a finish
            if (root.canonicalUrl !== "") {
                AppController.clearMovieResume(root.canonicalUrl);
            }
            // Auto-play the next episode/movie in the folder, if any
            var idx = AppController.channelViewModel.findIndexByUrl(root.canonicalUrl);
            var total = AppController.channelViewModel.rowCount();
            if (idx >= 0 && idx < total - 1) {
                osdOverlay.show("⏭ Next");
                root.playChannel(idx + 1);
            }
        }
    }

    function setQuality(q) {
        var cand = urlForQuality(root.streamUrl, q);
        if (cand === root.streamUrl) return;
        root.triedUrls = [cand]; // manual pick restarts the fallback chain
        osdOverlay.show(q + "p");
        startStream(cand);
    }

    // Auto-fallback: if the current URL fails, try the other qualities
    // (1080 → 720 → 480), skipping any URL already attempted.
    Connections {
        target: videoPlayer
        function onPlaybackFailed(reason) {
            if (root.detectQuality(root.streamUrl) === "") {
                osdOverlay.show("⚠ Playback error");
                return;
            }
            var order = ["1080", "720", "480"];
            for (var i = 0; i < order.length; i++) {
                var cand = root.urlForQuality(root.streamUrl, order[i]);
                if (root.triedUrls.indexOf(cand) === -1) {
                    root.triedUrls.push(cand);
                    osdOverlay.show("⚠ Failed — trying " + order[i] + "p…");
                    root.startStream(cand);
                    return;
                }
            }
            osdOverlay.show("⚠ All qualities failed");
        }
    }

    focus: true  // Enable keyboard input

    function toggleFullscreen() {
        var win = root.Window.window;
        if (root.isFullscreen) {
            // Leaving fullscreen: drop the frameless hint and put geometry back
            // via C++ (single native call, no intermediate OS animation).
            AppController.exitFullscreen(
                root.prevVisibility === Window.Maximized,
                root.prevX, root.prevY, root.prevWidth, root.prevHeight
            );
            root.isFullscreen = false;
        } else {
            // Remember exactly where/how the window was before going fullscreen.
            root.prevVisibility = win.visibility;
            root.prevX = win.x;
            root.prevY = win.y;
            root.prevWidth = win.width;
            root.prevHeight = win.height;
            // Entering fullscreen: just strip the frame and resize to the
            // screen's bounds. This never touches Qt::WindowFullScreen, so
            // Windows never runs its exclusive-fullscreen enter/exit animation
            // (the thing that was causing the flicker) — same technique VLC /
            // MPC-HC use for instant, flicker-free fullscreen.
            AppController.enterFullscreen();
            root.isFullscreen = true;
        }
    }

    // OSD (On-Screen Display) for seek/volume feedback
    Rectangle {
        id: osdOverlay
        anchors.centerIn: parent
        width: osdText.implicitWidth + 48
        height: 52
        radius: 26
        color: "#CC0d141d"
        border.color: "#33c2c6d2"
        border.width: 1
        z: 100
        opacity: 0
        visible: opacity > 0

        Text {
            id: osdText
            anchors.centerIn: parent
            color: "#ffb781"
            font.pixelSize: 18
            font.bold: true
        }

        Behavior on opacity { NumberAnimation { duration: 200 } }

        Timer {
            id: osdHideTimer
            interval: 800
            onTriggered: osdOverlay.opacity = 0
        }

        function show(msg) {
            osdText.text = msg;
            osdOverlay.opacity = 1;
            osdHideTimer.restart();
        }
    }

    // Seek acceleration timer (fires repeatedly while arrow key is held)
    Timer {
        id: seekTimer
        interval: 300
        repeat: true
        onTriggered: {
            if (root.seekingRight) {
                root.seekStep = Math.min(root.seekStep + 10, 120);
                videoPlayer.command(["seek", root.seekStep.toString()]);
                osdOverlay.show("▶▶ +" + root.seekStep + "s");
            } else if (root.seekingLeft) {
                root.seekStep = Math.min(root.seekStep + 10, 120);
                videoPlayer.command(["seek", "-" + root.seekStep.toString()]);
                osdOverlay.show("◀◀ -" + root.seekStep + "s");
            }
        }
    }

    // Space-bar hold timer: fires once space is held past the threshold,
    // boosting playback to 2x. A quick tap (release before this fires)
    // falls through to normal play/pause toggling instead.
    Timer {
        id: spaceHoldTimer
        interval: 350
        repeat: false
        onTriggered: {
            root.spaceHoldTriggered = true;
            videoPlayer.command(["set", "speed", "2.0"]);
            osdOverlay.show("⏩ 2x Speed");
        }
    }

    // Keyboard handler
    Keys.onPressed: function(event) {
        if (event.isAutoRepeat) {
            // Arrow keys held — start accelerated seeking
            if (event.key === Qt.Key_Right && !root.seekingRight) {
                root.seekingRight = true;
                root.seekStep = 10;
                seekTimer.start();
            } else if (event.key === Qt.Key_Left && !root.seekingLeft) {
                root.seekingLeft = true;
                root.seekStep = 10;
                seekTimer.start();
            }
            event.accepted = true;
            return;
        }

        switch (event.key) {
        case Qt.Key_Space:
            root.spaceHoldTriggered = false;
            spaceHoldTimer.restart();
            break;

        case Qt.Key_Right:
            videoPlayer.command(["seek", "10"]);
            osdOverlay.show("▶▶ +10s");
            break;

        case Qt.Key_Left:
            videoPlayer.command(["seek", "-10"]);
            osdOverlay.show("◀◀ -10s");
            break;

        case Qt.Key_Up:
            videoPlayer.volume = Math.min(videoPlayer.volume + 5, 200);
            osdOverlay.show("🔊 " + videoPlayer.volume + "%");
            break;

        case Qt.Key_Down:
            videoPlayer.volume = Math.max(videoPlayer.volume - 5, 0);
            osdOverlay.show("🔊 " + videoPlayer.volume + "%");
            break;

        case Qt.Key_F:
            root.toggleFullscreen();
            break;

        case Qt.Key_Escape:
            if (root.isFullscreen) {
                root.toggleFullscreen();
            } else if (root.playlistOpen) {
                root.playlistOpen = false;
            } else {
                root.backRequested();
            }
            break;

        case Qt.Key_M:
            if (videoPlayer.volume > 0) {
                root.savedVolume = videoPlayer.volume;
                videoPlayer.volume = 0;
                osdOverlay.show("🔇 Muted");
            } else {
                videoPlayer.volume = root.savedVolume;
                osdOverlay.show("🔊 " + videoPlayer.volume + "%");
            }
            break;

        case Qt.Key_N:
            root.playNext();
            osdOverlay.show("⏭ Next");
            break;

        case Qt.Key_P:
            root.playPrevious();
            osdOverlay.show("⏮ Previous");
            break;

        case Qt.Key_T:
            root.toggleAlwaysOnTop();
            break;

        case Qt.Key_L:
            root.playlistOpen = !root.playlistOpen;
            break;

        default:
            return; // Don't accept unhandled keys
        }
        event.accepted = true;
    }

    Keys.onReleased: function(event) {
        if (event.isAutoRepeat) {
            event.accepted = true;
            return;
        }

        // Stop accelerated seeking when arrow key is released
        if (event.key === Qt.Key_Right) {
            root.seekingRight = false;
            root.seekStep = 10;
            if (!root.seekingLeft) seekTimer.stop();
        } else if (event.key === Qt.Key_Left) {
            root.seekingLeft = false;
            root.seekStep = 10;
            if (!root.seekingRight) seekTimer.stop();
        } else if (event.key === Qt.Key_Space) {
            if (spaceHoldTimer.running) {
                // Released before the hold threshold — treat as a normal tap
                spaceHoldTimer.stop();
                videoPlayer.playing = !videoPlayer.playing;
                osdOverlay.show(videoPlayer.playing ? "▶ Play" : "⏸ Paused");
            } else if (root.spaceHoldTriggered) {
                // Was boosted to 2x — restore the user's selected speed
                root.spaceHoldTriggered = false;
                videoPlayer.command(["set", "speed", root.playbackSpeed.toString()]);
                osdOverlay.show(root.playbackSpeed + "x");
            }
        }
    }

    // Helper functions for navigation
    function playChannel(index) {

        var url = AppController.channelViewModel.channelStreamUrl(index);
        var name = AppController.channelViewModel.channelName(index);
        var referer = AppController.channelViewModel.channelReferer(index);
        var userAgent = AppController.channelViewModel.channelUserAgent(index);

        if (url !== "") {
            // Persist the outgoing video's position before switching
            root.saveResumeProgress();

            root.streamTitle = name;
            // Update root properties (the MpvVideoItem binds to these) BEFORE
            // the URL, so headers are already applied when loading starts.
            root.streamReferer = referer !== undefined ? referer : "";
            root.streamUserAgent = userAgent !== undefined ? userAgent : "";
            root.canonicalUrl = url;
            root.channelType = AppController.channelViewModel.channelType(index);
            // Movies/series resume from their saved point; live starts live.
            var resume = root.isVod ? AppController.getMovieResume(url) : { found: false };
            videoPlayer.startPosition = resume.found ? Math.floor(resume.positionMs / 1000) : 0;
            root.triedUrls = [url];
            root.spaceHoldTriggered = false;
            root.playbackSpeed = 1.0;
            videoPlayer.command(["set", "speed", "1.0"]);
            root.startStream(url);
        }
    }

    function playPrevious() {
        var idx = AppController.channelViewModel.findIndexByUrl(root.canonicalUrl);
        if (idx > 0) {
            playChannel(idx - 1);
        }
    }

    function playNext() {
        var idx = AppController.channelViewModel.findIndexByUrl(root.canonicalUrl);
        var total = AppController.channelViewModel.rowCount();
        if (idx >= 0 && idx < total - 1) {
            playChannel(idx + 1);
        }
    }

    // ===== Auto-hide controls =====
    // Controls (top bar + bottom control center) hide after 3s of no mouse
    // activity. Mouse movement shows them again; keyboard use does NOT.
    property bool controlsVisible: true

    function showControls() {
        root.controlsVisible = true;
        controlsHideTimer.restart();
    }

    Timer {
        id: controlsHideTimer
        // Settings → Controller Timeout (0.5s .. 10s)
        interval: Math.round(AppController.settings.controllerTimeout * 1000)
        onTriggered: {
            // Don't hide while the cursor is resting on the controls
            if (topBarHover.hovered || bottomBarHover.hovered) {
                controlsHideTimer.restart();
            } else {
                root.controlsVisible = false;
            }
        }
    }

    MpvVideoItem {
        id: videoPlayer
        anchors.fill: parent
        referer: root.streamReferer
        userAgent: root.streamUserAgent

        MouseArea {
            anchors.fill: parent
            hoverEnabled: true
            // Hide the cursor together with the controls (but keep it while
            // hovering the playlist hot-zone, where only that button shows)
            cursorShape: root.controlsVisible || playlistZoneHover.hovered ? Qt.ArrowCursor : Qt.BlankCursor
            onPositionChanged: {
                // Moving over the playlist hot-zone reveals only the playlist
                // button — don't wake the full control set from there.
                if (!playlistZoneHover.hovered)
                    root.showControls();
            }
            onClicked: {
                root.showControls();
                videoPlayer.playing = !videoPlayer.playing
                root.forceActiveFocus()
            }
            // Mouse wheel = volume up/down
            onWheel: function(wheel) {
                var step = wheel.angleDelta.y > 0 ? 5 : -5;
                videoPlayer.volume = Math.max(0, Math.min(videoPlayer.volume + step, 200));
                osdOverlay.show("🔊 " + videoPlayer.volume + "%");
            }
        }

        BusyIndicator {
            anchors.centerIn: parent
            running: videoPlayer.loading
            visible: running
            width: 64
            height: 64
        }
    }
    
    // Title Overlay
    Text {
        anchors.top: parent.top
        anchors.topMargin: 32
        anchors.horizontalCenter: parent.horizontalCenter
        text: root.streamTitle !== "" ? root.streamTitle.toUpperCase() : "NOW PLAYING"
        color: "#ffb781" // text-primary
        font.pixelSize: 14 // label-lg
        font.bold: true
        font.letterSpacing: 2.0
        z: 20
        style: Text.Outline; styleColor: "#80000000"
        opacity: root.controlsVisible ? 1.0 : 0.0
        visible: opacity > 0
        Behavior on opacity { NumberAnimation { duration: 250 } }
    }

    Text {
        anchors.top: parent.top
        anchors.right: parent.right
        anchors.margins: 10
        text: "Referer: " + (root.streamReferer ? root.streamReferer : "None")
        color: "#88FFFFFF"
        font.pixelSize: 10
        z: 20
        opacity: root.controlsVisible ? 1.0 : 0.0
        visible: opacity > 0
        Behavior on opacity { NumberAnimation { duration: 250 } }
    }
    
    // Top Bar Floating (back button only — playlist button lives in its own
    // layer below so it can appear without waking the full control set)
    RowLayout {
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.margins: 24
        z: 20

        // Auto-hide: fade with controlsVisible, ignore clicks when hidden
        opacity: root.controlsVisible ? 1.0 : 0.0
        visible: opacity > 0
        enabled: root.controlsVisible
        Behavior on opacity { NumberAnimation { duration: 250 } }

        HoverHandler {
            id: topBarHover
            onHoveredChanged: if (hovered) root.showControls()
        }

        // Back Button
        Button {
            id: backBtn
            implicitWidth: 48
            implicitHeight: 48
            background: Rectangle {
                color: backBtn.hovered ? "#2e3540" : "#992e3540" // bg-surface-container-highest/60
                radius: 24
                border.color: "#33c2c6d2"
                border.width: 1
            }
            contentItem: Text {
                text: "←"
                color: backBtn.hovered ? "#ffb781" : "#dce3f0"
                font.pixelSize: 24
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }
            onClicked: root.backRequested()
        }
    }

    // Playlist hot-zone: hovering the top-right corner reveals ONLY the
    // playlist button, even while the rest of the controls stay hidden.
    Item {
        anchors.top: parent.top
        anchors.right: parent.right
        width: 240
        height: 100
        z: 20

        HoverHandler {
            id: playlistZoneHover
        }
    }

    // Playlist Button — own layer so it can show independently of the bars
    Button {
        id: playlistBtn
        anchors.top: parent.top
        anchors.right: parent.right
        anchors.margins: 24
        implicitHeight: 48
        z: 21

        opacity: (root.controlsVisible || playlistZoneHover.hovered) ? 1.0 : 0.0
        visible: opacity > 0
        enabled: root.controlsVisible || playlistZoneHover.hovered
        Behavior on opacity { NumberAnimation { duration: 250 } }

        background: Rectangle {
            color: root.playlistOpen ? "#ff8800" : (playlistBtn.hovered ? "#2e3540" : "#992e3540")
            radius: 24
            border.color: root.playlistOpen ? "#ff8800" : "#33c2c6d2"
            border.width: 1
        }
        contentItem: RowLayout {
            spacing: 8
            Text {
                text: "≡"
                color: root.playlistOpen ? "#2f1400" : (playlistBtn.hovered ? "#ffb781" : "#dce3f0")
                font.pixelSize: 20
            }
            Text {
                text: "PLAYLIST"
                color: root.playlistOpen ? "#2f1400" : (playlistBtn.hovered ? "#ffb781" : "#dce3f0")
                font.pixelSize: 14
                font.bold: true
                font.letterSpacing: 1.0
            }
        }
        leftPadding: 16
        rightPadding: 20
        onClicked: root.playlistOpen = !root.playlistOpen
    }

    // ========== PLAYLIST PANEL (slide from right) ==========
    Rectangle {
        id: playlistPanel
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.right: parent.right
        width: 360
        color: "#E60d141d"
        border.color: "#33c2c6d2"
        border.width: 1
        z: 50
        visible: playlistPanelX.running || root.playlistOpen

        // Slide animation — driven by rightMargin (not x) so a window resize
        // doesn't retrigger the animation and flash the panel open/closed.
        anchors.rightMargin: root.playlistOpen ? 0 : -width
        Behavior on anchors.rightMargin {
            NumberAnimation {
                id: playlistPanelX
                duration: 280
                easing.type: Easing.OutCubic
            }
        }

        // Block mouse/wheel events from falling through to the video player
        MouseArea {
            anchors.fill: parent
            hoverEnabled: true
            onWheel: function(wheel) { wheel.accepted = true; }
            onClicked: function(mouse) { mouse.accepted = true; }
            onPressed: function(mouse) { mouse.accepted = true; }
        }

        // Panel Header
        ColumnLayout {
            anchors.fill: parent
            anchors.topMargin: 16
            spacing: 0

            // Header Row
            RowLayout {
                Layout.fillWidth: true
                Layout.leftMargin: 20
                Layout.rightMargin: 12
                Layout.bottomMargin: 12

                Text {
                    text: "PLAYLIST"
                    color: "#ffb781"
                    font.pixelSize: 14
                    font.bold: true
                    font.letterSpacing: 2.0
                    Layout.fillWidth: true
                }

                Text {
                    text: {
                        var idx = AppController.channelViewModel.findIndexByUrl(root.canonicalUrl);
                        var total = AppController.channelViewModel.rowCount();
                        if (idx >= 0) {
                            return (idx + 1) + " / " + total;
                        }
                        return total + " items";
                    }
                    color: "#7a8a9e"
                    font.pixelSize: 12
                }

                Button {
                    id: closePanelBtn
                    implicitWidth: 36
                    implicitHeight: 36
                    background: Rectangle { color: closePanelBtn.hovered ? "#2e3540" : "transparent"; radius: 18 }
                    contentItem: Text { text: "✕"; color: closePanelBtn.hovered ? "#ffb781" : "#7a8a9e"; font.pixelSize: 16; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                    onClicked: root.playlistOpen = false
                }
            }

            // Separator
            Rectangle {
                Layout.fillWidth: true
                height: 1
                color: "#33c2c6d2"
            }

            // Channel List
            ListView {
                id: playlistListView
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true
                model: AppController.channelViewModel

                // Auto-scroll to current channel when panel opens
                onVisibleChanged: {
                    if (visible) {
                        var idx = AppController.channelViewModel.findIndexByUrl(root.canonicalUrl);
                        if (idx >= 0) {
                            playlistListView.positionViewAtIndex(idx, ListView.Center);
                        }
                    }
                }

                delegate: Rectangle {
                    id: channelDelegate
                    width: playlistListView.width
                    height: 56
                    property bool isCurrent: model.streamUrl === root.canonicalUrl
                    color: isCurrent ? "#33ff8800" : (channelMouse.containsMouse ? "#1Ac2c6d2" : "transparent")

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 20
                        anchors.rightMargin: 20
                        spacing: 12

                        // Playing indicator or index number
                        Rectangle {
                            implicitWidth: 28
                            implicitHeight: 28
                            radius: 14
                            color: channelDelegate.isCurrent ? "#ff8800" : "transparent"
                            border.color: channelDelegate.isCurrent ? "#ff8800" : "#33c2c6d2"
                            border.width: channelDelegate.isCurrent ? 0 : 1

                            Text {
                                anchors.centerIn: parent
                                text: channelDelegate.isCurrent ? "▶" : (index + 1)
                                color: channelDelegate.isCurrent ? "#2f1400" : "#7a8a9e"
                                font.pixelSize: channelDelegate.isCurrent ? 12 : 11
                                font.bold: channelDelegate.isCurrent
                            }
                        }

                        // Channel name
                        Text {
                            text: model.name
                            color: channelDelegate.isCurrent ? "#ffb781" : "#dce3f0"
                            font.pixelSize: 14
                            font.bold: channelDelegate.isCurrent
                            elide: Text.ElideRight
                            Layout.fillWidth: true
                        }
                    }

                    // Bottom border
                    Rectangle {
                        anchors.bottom: parent.bottom
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.leftMargin: 20
                        anchors.rightMargin: 20
                        height: 1
                        color: "#1Ac2c6d2"
                    }

                    MouseArea {
                        id: channelMouse
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            root.playChannel(index);
                        }
                    }
                }

                ScrollBar.vertical: ScrollBar {
                    policy: ScrollBar.AsNeeded
                    width: 6
                    contentItem: Rectangle {
                        implicitWidth: 6
                        radius: 3
                        color: "#55c2c6d2"
                    }
                }
            }
        }
    }

    // Bottom Controls Layer
    Item {
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottomMargin: 24
        height: 100
        z: 30

        // Auto-hide: fade with controlsVisible, ignore clicks when hidden
        opacity: root.controlsVisible ? 1.0 : 0.0
        visible: opacity > 0
        enabled: root.controlsVisible
        Behavior on opacity { NumberAnimation { duration: 250 } }

        HoverHandler {
            id: bottomBarHover
            onHoveredChanged: if (hovered) root.showControls()
        }
        
        // Glassmorphic Panel
        Rectangle {
            anchors.centerIn: parent
            width: Math.min(parent.width - 48, 1152) // max-w-6xl
            height: 96
            color: "#B30d141d" // bg-surface/70 (slightly more opaque for readability)
            radius: 12
            border.color: "#33c2c6d2" // border-tertiary/20
            border.width: 1
            
            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 24
                anchors.rightMargin: 24
                spacing: 20
                
                // --- Left: Secondary Controls ---
                RowLayout {
                    spacing: 8
                    
                    // Subtitles Button
                    Button {
                        id: subBtn
                        implicitWidth: 40
                        implicitHeight: 40
                        background: Rectangle { color: subBtn.hovered ? "#232a35" : "transparent"; radius: 20 }
                        contentItem: Text { text: "💬"; color: subBtn.hovered ? "#dce3f0" : "#dec1ae"; font.pixelSize: 18; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                        onClicked: subMenu.open()
                        Menu {
                            id: subMenu
                            y: -height - 10
                            MenuItem { text: "Subtitles"; enabled: false }
                            MenuItem {
                                text: "Off"
                                onTriggered: videoPlayer.setTrack("sub", -1)
                            }
                            Repeater {
                                model: videoPlayer.subtitleTracks
                                MenuItem {
                                    text: (modelData.lang ? "[" + modelData.lang + "] " : "") + (modelData.title ? modelData.title : ("Track " + modelData.id))
                                    onTriggered: videoPlayer.setTrack("sub", modelData.id)
                                }
                            }
                        }
                    }
                    
                    // Audio Tracks Button
                    Button {
                        id: audioBtn
                        implicitWidth: 40
                        implicitHeight: 40
                        background: Rectangle { color: audioBtn.hovered ? "#232a35" : "transparent"; radius: 20 }
                        contentItem: Text { text: "🎵"; color: audioBtn.hovered ? "#dce3f0" : "#dec1ae"; font.pixelSize: 18; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                        onClicked: audioMenu.open()
                        Menu {
                            id: audioMenu
                            y: -height - 10
                            MenuItem { text: "Audio Tracks"; enabled: false }
                            Repeater {
                                model: videoPlayer.audioTracks
                                MenuItem {
                                    text: (modelData.lang ? "[" + modelData.lang + "] " : "") + (modelData.title ? modelData.title : ("Track " + modelData.id))
                                    onTriggered: videoPlayer.setTrack("audio", modelData.id)
                                }
                            }
                        }
                    }

                    // Quality Button — only shown when the URL has a
                    // recognizable 1080p/720p/480p token to rewrite.
                    Button {
                        id: qualityBtn
                        visible: root.detectQuality(root.streamUrl) !== ""
                        implicitHeight: 40
                        leftPadding: 12
                        rightPadding: 12
                        background: Rectangle {
                            color: qualityBtn.hovered ? "#232a35" : "transparent"
                            radius: 20
                            border.color: "#33c2c6d2"
                            border.width: 1
                        }
                        contentItem: Text {
                            text: root.detectQuality(root.streamUrl) + "p ▾"
                            color: qualityBtn.hovered ? "#dce3f0" : "#dec1ae"
                            font.pixelSize: 13
                            font.bold: true
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                        onClicked: qualityMenu.open()
                        Menu {
                            id: qualityMenu
                            y: -height - 10
                            MenuItem { text: "Quality"; enabled: false }
                            Repeater {
                                model: ["1080", "720", "480"]
                                MenuItem {
                                    text: modelData + "p" + (root.detectQuality(root.streamUrl) === modelData ? "   ✓" : "")
                                    onTriggered: root.setQuality(modelData)
                                }
                            }
                        }
                    }

                    // Playback Speed Button
                    Button {
                        id: speedBtn
                        implicitHeight: 40
                        leftPadding: 12
                        rightPadding: 12
                        background: Rectangle {
                            color: speedBtn.hovered ? "#232a35" : "transparent"
                            radius: 20
                            border.color: "#33c2c6d2"
                            border.width: 1
                        }
                        contentItem: Text {
                            text: root.playbackSpeed + "x ▾"
                            color: speedBtn.hovered ? "#dce3f0" : "#dec1ae"
                            font.pixelSize: 13
                            font.bold: true
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                        onClicked: speedMenu.open()
                        Menu {
                            id: speedMenu
                            y: -height - 10
                            MenuItem { text: "Playback Speed"; enabled: false }
                            Repeater {
                                model: [0.25, 0.5, 0.75, 1.0, 1.25, 1.5, 1.75, 2.0]
                                MenuItem {
                                    text: modelData + "x" + (root.playbackSpeed === modelData ? "   ✓" : "")
                                    onTriggered: root.setPlaybackSpeed(modelData)
                                }
                            }
                        }
                    }
                }
                
                // --- Center: Playback Controls & Progress ---
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 4
                    
                    RowLayout {
                        Layout.alignment: Qt.AlignCenter
                        spacing: 16
                        
                        // Prev
                        Button {
                            id: prevBtn
                            implicitWidth: 40
                            implicitHeight: 40
                            background: Rectangle { color: prevBtn.hovered ? "#232a35" : "transparent"; radius: 20 }
                            contentItem: Item {
                                anchors.fill: parent
                                Image {
                                    anchors.centerIn: parent
                                    sourceSize: Qt.size(24, 24)
                                    source: "data:image/svg+xml;utf8,<svg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 24 24' width='24' height='24' fill='" + (prevBtn.hovered ? "%23dce3f0" : "%23dec1ae") + "'><path d='M6 6h2v12H6zm3.5 6l8.5 6V6z'/></svg>"
                                }
                            }
                            onClicked: root.playPrevious()
                        }
                        
                        // Play/Pause (Primary)
                        Button {
                            id: playBtn
                            implicitWidth: 52
                            implicitHeight: 52
                            background: Rectangle {
                                color: playBtn.hovered ? "#ffb781" : "#FF8800"
                                radius: 26
                            }
                            contentItem: Item {
                                anchors.fill: parent
                                Image {
                                    anchors.centerIn: parent
                                    sourceSize: Qt.size(28, 28)
                                    source: {
                                        let c = "%232f1400";
                                        if (videoPlayer.playing) {
                                            return "data:image/svg+xml;utf8,<svg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 24 24' width='28' height='28' fill='" + c + "'><path d='M6 19h4V5H6v14zm8-14v14h4V5h-4z'/></svg>";
                                        } else {
                                            return "data:image/svg+xml;utf8,<svg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 24 24' width='28' height='28' fill='" + c + "'><path d='M8 5v14l11-7z'/></svg>";
                                        }
                                    }
                                }
                            }
                            onClicked: videoPlayer.playing = !videoPlayer.playing
                        }
                        
                        // Next
                        Button {
                            id: nextBtn
                            implicitWidth: 40
                            implicitHeight: 40
                            background: Rectangle { color: nextBtn.hovered ? "#232a35" : "transparent"; radius: 20 }
                            contentItem: Item {
                                anchors.fill: parent
                                Image {
                                    anchors.centerIn: parent
                                    sourceSize: Qt.size(24, 24)
                                    source: "data:image/svg+xml;utf8,<svg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 24 24' width='24' height='24' fill='" + (nextBtn.hovered ? "%23dce3f0" : "%23dec1ae") + "'><path d='M6 18l8.5-6L6 6v12zM16 6v12h2V6h-2z'/></svg>"
                                }
                            }
                            onClicked: root.playNext()
                        }
                    }
                    
                    // Simplified Progress Bar
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 12
                        
                        Text {
                            text: {
                                let p = videoPlayer.position;
                                let fmt = function(s) {
                                    let m = Math.floor(s / 60);
                                    let sc = Math.floor(s % 60);
                                    return m + ":" + (sc < 10 ? "0" : "") + sc;
                                }
                                return fmt(p);
                            }
                            color: "#dec1ae"
                            font.pixelSize: 11
                        }
                        
                        Slider {
                            id: progressSlider
                            Layout.fillWidth: true
                            Layout.preferredHeight: 12
                            
                            from: 0
                            to: videoPlayer.duration > 0 ? videoPlayer.duration : 1
                            value: videoPlayer.position
                            
                            onMoved: videoPlayer.position = value
                            
                            background: Rectangle {
                                id: progBg
                                x: progressSlider.leftPadding
                                y: progressSlider.topPadding + progressSlider.availableHeight / 2 - height / 2
                                width: progressSlider.availableWidth
                                height: 4
                                radius: 2
                                color: "#2e3540" // surface-container-highest
                                
                                Rectangle {
                                    width: progressSlider.visualPosition * progBg.width
                                    height: progBg.height
                                    color: "#ffb781" // primary
                                    radius: 2
                                }
                            }
                            handle: Rectangle {
                                x: progressSlider.leftPadding + progressSlider.visualPosition * (progressSlider.availableWidth - width) - width/2
                                y: progressSlider.topPadding + progressSlider.availableHeight / 2 - height / 2
                                implicitWidth: 10
                                implicitHeight: 10
                                radius: 5
                                color: "#ffb781"
                                opacity: progressSlider.pressed || progressSlider.hovered ? 1.0 : 0.0
                                Behavior on opacity { NumberAnimation { duration: 150 } }
                            }
                        }
                        
                        Text {
                            text: {
                                let d = videoPlayer.duration;
                                let fmt = function(s) {
                                    let m = Math.floor(s / 60);
                                    let sc = Math.floor(s % 60);
                                    return m + ":" + (sc < 10 ? "0" : "") + sc;
                                }
                                return fmt(d);
                            }
                            color: "#dec1ae"
                            font.pixelSize: 11
                        }
                    }
                }
                
                // --- Right: Volume / Settings ---
                RowLayout {
                    spacing: 8
                    
                    Text {
                        text: "🔊"
                        color: "#dec1ae"
                        font.pixelSize: 16
                    }
                    
                    Slider {
                        id: volSlider
                        from: 0
                        to: 200
                        value: videoPlayer.volume
                        Layout.preferredWidth: 80
                        onMoved: {
                            videoPlayer.volume = value
                        }
                        background: Rectangle {
                            id: volBg
                            x: volSlider.leftPadding
                            y: volSlider.topPadding + volSlider.availableHeight / 2 - height / 2
                            width: volSlider.availableWidth
                            height: 4
                            radius: 2
                            color: "#2e3540"
                            Rectangle {
                                width: volSlider.visualPosition * volBg.width
                                height: volBg.height
                                color: "#ffb781"
                                radius: 2
                            }
                        }
                        handle: Rectangle {
                            x: volSlider.leftPadding + volSlider.visualPosition * (volSlider.availableWidth - width) - width/2
                            y: volSlider.topPadding + volSlider.availableHeight / 2 - height / 2
                            implicitWidth: 10
                            implicitHeight: 10
                            radius: 5
                            color: "#ffb781"
                            opacity: volSlider.pressed || volSlider.hovered ? 1.0 : 0.0
                            Behavior on opacity { NumberAnimation { duration: 150 } }
                        }
                    }
                    
                    // Always on Top (pin) — VLC-style: keeps the app window
                    // above every other application while enabled.
                    Button {
                        id: pinBtn
                        implicitWidth: 40
                        implicitHeight: 40
                        background: Rectangle {
                            color: root.alwaysOnTop ? "#ff8800" : (pinBtn.hovered ? "#232a35" : "transparent")
                            radius: 20
                            border.color: root.alwaysOnTop ? "#ff8800" : "transparent"
                            border.width: 1
                        }
                        contentItem: Text {
                            text: "📌"
                            color: root.alwaysOnTop ? "#2f1400" : (pinBtn.hovered ? "#dce3f0" : "#dec1ae")
                            font.pixelSize: 16
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                        ToolTip.visible: pinBtn.hovered
                        ToolTip.delay: 600
                        ToolTip.text: "Always on Top (T)"
                        onClicked: root.toggleAlwaysOnTop()
                    }

                    Button {
                        id: fullscreenBtn
                        implicitWidth: 40
                        implicitHeight: 40
                        background: Rectangle { color: fullscreenBtn.hovered ? "#232a35" : "transparent"; radius: 20 }
                        contentItem: Text { text: "⛶"; color: fullscreenBtn.hovered ? "#dce3f0" : "#dec1ae"; font.pixelSize: 18; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                        onClicked: {
                            root.toggleFullscreen();
                        }
                    }
                }
            }
        }
    }
    
    Component.onCompleted: {
        if (root.canonicalUrl === "") root.canonicalUrl = root.streamUrl;
        // Resume seek is applied by MpvVideoItem once the file has loaded
        if (root.resumePositionMs > 0) {
            videoPlayer.startPosition = Math.floor(root.resumePositionMs / 1000);
        }
        root.triedUrls = [root.streamUrl];
        videoPlayer.mediaUrl = root.streamUrl
        videoPlayer.playing = true
        root.forceActiveFocus()
    }

    Component.onDestruction: {
        // Final position save when leaving the player
        root.saveResumeProgress();
        // Release always-on-top — the toggle lives in this view, so leaving
        // the player must not leave the window stuck topmost.
        if (root.alwaysOnTop) AppController.setAlwaysOnTop(false);
    }
}
