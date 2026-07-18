import QtQuick
import QtQuick.Controls
import M3uVideoPlayer

// MX-Player-style floating play button: resumes the playlist's last-played
// video (movies with time skip, live TV from the start). Hidden until the
// playlist has a resume row.
Item {
    id: root

    property int playlistId: -1
    // Last-played row from AppController.getPlaylistResume(); null = none
    property var resumeInfo: null

    signal playRequested(var resume)

    width: 56
    height: 56
    z: 50
    visible: resumeInfo !== null

    function refresh() {
        if (playlistId > 0) {
            var row = AppController.getPlaylistResume(playlistId);
            resumeInfo = row.found ? row : null;
        } else {
            resumeInfo = null;
        }
    }

    onPlaylistIdChanged: refresh()

    function formatTime(ms) {
        var totalSec = Math.floor(ms / 1000);
        var h = Math.floor(totalSec / 3600);
        var m = Math.floor((totalSec % 3600) / 60);
        var s = totalSec % 60;
        var mm = (m < 10 && h > 0 ? "0" : "") + m;
        var ss = (s < 10 ? "0" : "") + s;
        return h > 0 ? h + ":" + mm + ":" + ss : m + ":" + ss;
    }

    Button {
        id: fabBtn
        anchors.fill: parent
        scale: fabBtn.hovered ? 1.08 : 1.0
        Behavior on scale { NumberAnimation { duration: 120 } }

        background: Rectangle {
            color: fabBtn.hovered ? "#FFE082" : "#FFD54F"
            radius: width / 2
            border.color: "#66000000"
            border.width: 1
        }
        contentItem: Text {
            text: "▶"
            color: "#121212"
            font.pixelSize: 22
            font.bold: true
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
        }

        ToolTip.visible: fabBtn.hovered && root.resumeInfo !== null
        ToolTip.delay: 300
        ToolTip.text: root.resumeInfo === null ? "" :
            ("Resume: " + root.resumeInfo.title
             + (root.resumeInfo.positionMs > 0 ? "  (" + root.formatTime(root.resumeInfo.positionMs) + ")" : ""))

        onClicked: {
            if (root.resumeInfo !== null) {
                root.playRequested(root.resumeInfo);
            }
        }
    }
}
