import QtQuick
import QtQuick.Window
import "screens"

// Detached VLC-style player window: every extra video (local file opened
// while something is already playing, or a channel middle-clicked in a list)
// plays in its own independent Window. It has no visual parent — closing the
// main window or any other player window never affects it, and it is
// destroyed only when it is closed itself. When the last open window closes,
// Qt quits the application automatically.
Window {
    id: playerWindow
    width: 1024
    height: 576
    minimumWidth: 480
    minimumHeight: 270
    color: "#121212"
    title: player.streamTitle !== "" ? player.streamTitle : qsTr("M3U Video Player")

    property alias streamUrl: player.streamUrl
    property alias streamTitle: player.streamTitle
    property alias streamReferer: player.streamReferer
    property alias streamUserAgent: player.streamUserAgent
    property alias channelType: player.channelType
    property alias playlistId: player.playlistId
    property alias groupId: player.groupId
    property alias groupTitle: player.groupTitle
    property alias resumePositionMs: player.resumePositionMs
    property alias channelModel: player.channelModel

    // Lets main.qml drop its bookkeeping reference so the JS GC can collect
    // this window (and, with it, the injected channel model).
    signal windowClosed()

    PlayerView {
        id: player
        anchors.fill: parent
        // Defaults for local files: MOVIE seek behavior, no history/resume
        // writes. Channels opened in a new window override these with their
        // real type/playlist/group so resume keeps working there.
        channelType: 1
        playlistId: 0
        groupId: 0
        onBackRequested: playerWindow.close()
    }

    onClosing: {
        playerWindow.windowClosed()
        // destroy() is deferred to the next event-loop pass, safely after the
        // close event: tears down PlayerView -> MpvVideoItem destructor ->
        // clean mpv handle shutdown (same path as a StackView pop).
        playerWindow.destroy()
    }
}
