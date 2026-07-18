import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import M3uVideoPlayer

// Edit an existing playlist's name / source link / update frequency
// (opened from Settings → Playlists).
Popup {
    id: dialog
    width: 480
    height: contentColumn.implicitHeight + 48
    modal: true
    focus: true
    anchors.centerIn: parent

    property int playlistId: -1
    property bool isXtream: false
    property string playlistUsername: ""
    property string playlistPassword: ""

    function openFor(id, name, url, frequency, xtream, username, password) {
        playlistId = id;
        isXtream = xtream;
        playlistUsername = username || "";
        playlistPassword = password || "";
        nameField.text = name;
        urlField.text = url;
        // enum values: EVERY_STARTUP=1, EVERY_6_HOURS=2, EVERY_12_HOURS=3, EVERY_3_DAYS=4, WEEKLY=5, NEVER=0
        var idx = freqCombo.freqValues.indexOf(frequency);
        freqCombo.currentIndex = idx >= 0 ? idx : 0;
        open();
    }

    signal saved()

    background: Rectangle {
        color: "#1E1E1E"
        radius: 12
        border.color: "#333333"
        border.width: 1
    }

    ColumnLayout {
        id: contentColumn
        anchors.fill: parent
        anchors.margins: 20
        spacing: 14

        RowLayout {
            Layout.fillWidth: true
            Text {
                text: "Edit Playlist"
                color: "#FFD54F"
                font.pixelSize: 18
                font.bold: true
                Layout.fillWidth: true
            }
            Button {
                text: "✕"
                background: null
                contentItem: Text {
                    text: parent.text
                    color: parent.hovered ? "#FFD54F" : "#AAAAAA"
                    font.pixelSize: 18
                }
                onClicked: dialog.close()
            }
        }

        ColumnLayout {
            spacing: 6
            Layout.fillWidth: true
            Text { text: "Playlist Name"; color: "#AAAAAA"; font.pixelSize: 13 }
            TextField {
                id: nameField
                Layout.fillWidth: true
                color: "#FFFFFF"
                background: Rectangle {
                    color: "#121212"; radius: 8; implicitHeight: 42
                    border.color: nameField.activeFocus ? "#FFD54F" : "#333333"
                }
                leftPadding: 12
            }
        }

        ColumnLayout {
            spacing: 6
            Layout.fillWidth: true
            Text {
                text: dialog.isXtream ? "Server URL" : "Source Link (URL or file path)"
                color: "#AAAAAA"; font.pixelSize: 13
            }
            TextField {
                id: urlField
                Layout.fillWidth: true
                color: "#FFFFFF"
                background: Rectangle {
                    color: "#121212"; radius: 8; implicitHeight: 42
                    border.color: urlField.activeFocus ? "#FFD54F" : "#333333"
                }
                leftPadding: 12
            }
        }

        ColumnLayout {
            spacing: 6
            Layout.fillWidth: true
            Text { text: "How Often to Update"; color: "#AAAAAA"; font.pixelSize: 13 }
            ComboBox {
                id: freqCombo
                Layout.fillWidth: true
                model: ["On application start", "Every 6 hours", "Every 12 hours", "Every 3 days", "Every week", "Never"]
                readonly property var freqValues: [1, 2, 3, 4, 5, 0]
                readonly property int selectedFrequency: freqValues[currentIndex]

                background: Rectangle {
                    color: "#121212"; radius: 8; implicitHeight: 42
                    border.color: freqCombo.activeFocus ? "#FFD54F" : "#333333"
                }
                contentItem: Text {
                    text: freqCombo.displayText
                    color: "#FFFFFF"
                    font.pixelSize: 14
                    verticalAlignment: Text.AlignVCenter
                    leftPadding: 12
                    elide: Text.ElideRight
                }
                indicator: Text {
                    x: freqCombo.width - width - 14
                    y: freqCombo.topPadding + (freqCombo.availableHeight - height) / 2
                    text: "▾"
                    color: "#AAAAAA"
                    font.pixelSize: 14
                }
                delegate: ItemDelegate {
                    id: freqDelegate
                    required property int index
                    required property string modelData
                    width: freqCombo.width
                    height: 38
                    highlighted: freqCombo.highlightedIndex === index
                    background: Rectangle { color: freqDelegate.highlighted ? "#333333" : "#121212" }
                    contentItem: Text {
                        text: freqDelegate.modelData
                        color: freqCombo.currentIndex === freqDelegate.index ? "#FFD54F" : "#FFFFFF"
                        font.pixelSize: 14
                        verticalAlignment: Text.AlignVCenter
                        leftPadding: 8
                    }
                }
                popup: Popup {
                    y: freqCombo.height + 4
                    width: freqCombo.width
                    implicitHeight: contentItem.implicitHeight + 2
                    padding: 1
                    background: Rectangle {
                        color: "#121212"; radius: 8
                        border.color: "#333333"; border.width: 1
                    }
                    contentItem: ListView {
                        clip: true
                        implicitHeight: contentHeight
                        model: freqCombo.popup.visible ? freqCombo.delegateModel : null
                        currentIndex: freqCombo.highlightedIndex
                    }
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.topMargin: 6
            spacing: 12

            Item { Layout.fillWidth: true }

            Button {
                text: "Cancel"
                background: Rectangle { color: "transparent"; radius: 6; implicitHeight: 38; implicitWidth: 90 }
                contentItem: Text {
                    text: parent.text; color: parent.hovered ? "#FFFFFF" : "#AAAAAA"
                    font.pixelSize: 14
                    horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter
                }
                onClicked: dialog.close()
            }

            Button {
                text: "Save"
                background: Rectangle {
                    color: "#FFD54F"; radius: 6; implicitHeight: 38; implicitWidth: 100
                    opacity: parent.hovered ? 0.9 : 1.0
                }
                contentItem: Text {
                    text: parent.text; color: "#121212"; font.bold: true; font.pixelSize: 14
                    horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter
                }
                onClicked: {
                    if (AppController.playlistViewModel.updatePlaylistMeta(
                            dialog.playlistId, nameField.text, urlField.text,
                            freqCombo.selectedFrequency,
                            dialog.playlistUsername, dialog.playlistPassword)) {
                        dialog.saved();
                    }
                    dialog.close();
                }
            }
        }
    }
}
