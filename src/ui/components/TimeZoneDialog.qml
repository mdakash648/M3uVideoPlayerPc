import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import M3uVideoPlayer

// Time zone picker: searchable IANA list (with UTC offsets) plus an
// "Auto detect by location" action that resolves the zone from the
// public IP — independent of the PC's own (possibly wrong) settings.
Popup {
    id: dialog
    width: 480
    height: 560
    modal: true
    focus: true
    anchors.centerIn: parent

    background: Rectangle {
        color: "#1E1E1E"
        radius: 12
        border.color: "#333333"
        border.width: 1
    }

    property var allZones: []

    function applyFilter() {
        const query = searchField.text.toLowerCase();
        zoneModel.clear();
        for (var i = 0; i < allZones.length; i++) {
            if (query === "" || allZones[i].label.toLowerCase().indexOf(query) !== -1) {
                zoneModel.append(allZones[i]);
            }
        }
    }

    onOpened: {
        // The full list is a few hundred entries — load once per open
        allZones = AppController.settings.availableTimeZones();
        searchField.text = "";
        applyFilter();
        searchField.forceActiveFocus();
    }

    ListModel { id: zoneModel }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 16
        spacing: 12

        RowLayout {
            Layout.fillWidth: true
            Text {
                text: "Select Time Zone"
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

        // Auto-detect by location
        Button {
            id: autoDetectBtn
            Layout.fillWidth: true
            enabled: !detectBusy.running
            background: Rectangle {
                color: parent.hovered ? "#333333" : "#2A2A2A"
                radius: 8
                border.color: autoDetectBtn.activeFocus ? "#FFD54F" : "#444444"
            }
            contentItem: RowLayout {
                spacing: 8
                Text { text: "📍"; font.pixelSize: 15 }
                Text {
                    text: detectBusy.running ? "Detecting…" : "Auto detect by location"
                    color: "#FFFFFF"
                    font.pixelSize: 14
                    font.bold: true
                    Layout.fillWidth: true
                }
                BusyIndicator {
                    id: detectBusy
                    running: false
                    implicitWidth: 20
                    implicitHeight: 20
                }
            }
            padding: 12
            onClicked: {
                detectBusy.running = true;
                AppController.settings.autoDetectTimeZone();
            }
        }

        Connections {
            target: AppController.settings
            function onAutoDetectFinished(success, zoneId) {
                detectBusy.running = false;
                if (success) dialog.close();
            }
        }

        // Search
        TextField {
            id: searchField
            Layout.fillWidth: true
            placeholderText: "Search… e.g. utc-10, utc+06, Dhaka"
            placeholderTextColor: "#666666"
            color: "#FFFFFF"
            background: Rectangle {
                color: "#121212"
                radius: 8
                implicitHeight: 42
                border.color: searchField.activeFocus ? "#FFD54F" : "#333333"
            }
            leftPadding: 12
            onTextChanged: dialog.applyFilter()
        }

        // Zone list
        ListView {
            id: zoneList
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            spacing: 4
            model: zoneModel

            ScrollBar.vertical: ScrollBar { }

            delegate: Rectangle {
                width: zoneList.width
                height: 40
                radius: 6
                readonly property bool isSelected:
                    !AppController.settings.timeZoneAuto
                    && AppController.settings.timeZoneId === model.id
                color: zoneHover.containsMouse ? "#333333" : (isSelected ? "#2A2A2A" : "transparent")
                border.color: isSelected ? "#FFD54F" : "transparent"
                border.width: 1

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 12
                    anchors.rightMargin: 12
                    Text {
                        text: model.label
                        color: parent.parent.isSelected ? "#FFD54F" : "#FFFFFF"
                        font.pixelSize: 13
                        elide: Text.ElideRight
                        Layout.fillWidth: true
                    }
                    Text {
                        visible: parent.parent.isSelected
                        text: "✓"
                        color: "#FFD54F"
                        font.pixelSize: 14
                        font.bold: true
                    }
                }

                MouseArea {
                    id: zoneHover
                    anchors.fill: parent
                    hoverEnabled: true
                    onClicked: {
                        AppController.settings.timeZoneAuto = false;
                        AppController.settings.timeZoneId = model.id;
                        dialog.close();
                    }
                }
            }
        }
    }
}
