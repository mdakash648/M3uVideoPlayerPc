import QtQuick

Rectangle {
    id: root

    property string source: ""
    property string fallbackText: "LOGO"

    color: "#2A2A2A"
    radius: 4
    clip: true

    Image {
        id: img
        anchors.fill: parent
        anchors.margins: 4
        source: root.source.length > 0 ? root.source : ""
        fillMode: Image.PreserveAspectFit
        asynchronous: true
        cache: true
        visible: root.source.length > 0 && status === Image.Ready
    }

    // Busy indicator while the logo is downloading
    Text {
        anchors.centerIn: parent
        visible: root.source.length > 0 && img.status === Image.Loading
        text: "…"
        color: "#777777"
        font.pixelSize: 14
    }

    // Fallback shown when there is no tvg-logo, or it failed to load
    Text {
        anchors.centerIn: parent
        visible: root.source.length === 0 || img.status === Image.Error
        text: root.fallbackText
        color: "#555555"
        font.pixelSize: 12
    }
}
