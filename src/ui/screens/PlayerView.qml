import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import M3uVideoPlayer

Item {
    id: root
    
    property string streamUrl: ""
    signal backRequested()

    MpvVideoItem {
        id: videoPlayer
        anchors.fill: parent
        
        mediaUrl: root.streamUrl
        
        MouseArea {
            anchors.fill: parent
            onClicked: {
                videoPlayer.playing = !videoPlayer.playing
            }
        }
    }
    
    // Bottom Control Bar
    Rectangle {
        id: bottomBar
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        height: 60
        color: "#AA121212"
        
        RowLayout {
            anchors.fill: parent
            anchors.margins: 10
            
            Button {
                text: "← Back"
                onClicked: root.backRequested()

                contentItem: Text {
                    text: parent.text
                    color: "#FFD54F"
                    font.bold: true
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                background: Rectangle { color: "transparent" }
            }

            Button {
                text: videoPlayer.playing ? "Pause" : "Play"
                onClicked: videoPlayer.playing = !videoPlayer.playing
                
                contentItem: Text {
                    text: parent.text
                    color: "white"
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                background: Rectangle {
                    color: parent.pressed ? "#555" : "transparent"
                    radius: 4
                }
            }
            
            Text {
                text: {
                    let p = videoPlayer.position;
                    let d = videoPlayer.duration;
                    
                    let fmt = function(s) {
                        let m = Math.floor(s / 60);
                        let sc = Math.floor(s % 60);
                        return m + ":" + (sc < 10 ? "0" : "") + sc;
                    }
                    
                    return fmt(p) + " / " + fmt(d);
                }
                color: "white"
                Layout.alignment: Qt.AlignVCenter
            }
            
            Slider {
                Layout.fillWidth: true
                from: 0
                to: videoPlayer.duration > 0 ? videoPlayer.duration : 1
                value: videoPlayer.position
                
                onMoved: {
                    videoPlayer.position = value
                }
            }
            
            Text {
                text: "Vol: " + Math.round(videoPlayer.volume) + "%"
                color: "white"
                Layout.alignment: Qt.AlignVCenter
            }
            
            Slider {
                from: 0
                to: 130
                value: videoPlayer.volume
                onMoved: {
                    videoPlayer.volume = value
                }
            }
            
            Button {
                text: "⚙"
                onClicked: qualityMenu.open()
                contentItem: Text {
                    text: parent.text
                    color: "white"
                    font.pixelSize: 20
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                background: Rectangle { color: "transparent" }
                
                Menu {
                    id: qualityMenu
                    y: -height
                    
                    MenuItem {
                        text: "--- Video Quality ---"
                        enabled: false
                    }
                    
                    Repeater {
                        model: videoPlayer.videoTracks
                        MenuItem {
                            text: modelData.title ? modelData.title : ("Track " + modelData.id)
                            onTriggered: videoPlayer.setTrack("video", modelData.id)
                        }
                    }
                    
                    MenuItem {
                        text: "--- Audio Tracks ---"
                        enabled: false
                    }
                    
                    Repeater {
                        model: videoPlayer.audioTracks
                        MenuItem {
                            text: (modelData.lang ? "[" + modelData.lang + "] " : "") + (modelData.title ? modelData.title : ("Track " + modelData.id))
                            onTriggered: videoPlayer.setTrack("audio", modelData.id)
                        }
                    }
                }
            }
        }
    }
    
    Component.onCompleted: {
        videoPlayer.playing = true
    }
}
