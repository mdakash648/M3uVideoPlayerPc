import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import QtCore
import M3uVideoPlayer
import "../components"

Rectangle {
    id: root
    color: "#121212"

    signal playlistsRequested()

    function focusHeader() { focusMain(); }
    function focusMain() {
        if (settingsList.currentIndex < 0 && settingsList.count > 0) settingsList.currentIndex = 0;
        settingsList.forceActiveFocus();
    }

    StackView.onActivated: focusMain()

    // ===== Toast for action feedback =====
    function showToast(message) {
        toastText.text = message;
        toast.opacity = 1.0;
        toastTimer.restart();
    }

    Connections {
        target: AppController
        function onRefreshAllProgress(done, total, name) {
            if (total > 0) root.showToast("Updating playlists… " + done + "/" + total);
        }
        function onRefreshAllFinished(succeeded, failed) {
            if (succeeded === 0 && failed === 0) {
                root.showToast("No playlists to update");
            } else {
                root.showToast("Updated " + succeeded + " playlist(s)"
                               + (failed > 0 ? (", " + failed + " failed") : ""));
            }
        }
        function onBackupFinished(success, message) { root.showToast(message); }
        function onRestoreFinished(success, message) { root.showToast(message); }
        function onHistoryExportFinished(success, message) { root.showToast(message); }
        function onHistoryImportFinished(success, message) { root.showToast(message); }
    }

    Connections {
        target: AppController.settings
        function onAutoDetectFinished(success, zoneId) {
            root.showToast(success ? "Time zone set to " + zoneId
                                   : "Could not detect time zone");
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 24
        spacing: 20

        Text {
            text: qsTr("Settings")
            color: "#FFD54F"
            font.pixelSize: 24
            font.bold: true
        }

        ListView {
            id: settingsList
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            spacing: 10
            focus: true

            // type: "nav" rows trigger their action on click/Enter;
            // "gridColumns"/"posterColumns"/"timeout" rows carry an inline control.
            model: ListModel {
                ListElement { icon: "≡";  title: "Playlists";               subtitle: "View, edit and remove your saved playlists";                 type: "playlists" }
                ListElement { icon: "⟳";  title: "Update Playlists";        subtitle: "Refresh all playlists from their source";                    type: "updateAll" }
                ListElement { icon: "🖫";  title: "Backup & Restore";        subtitle: "Save or restore your app data";                              type: "backup" }
                ListElement { icon: "↥";  title: "Export History Playlist"; subtitle: "Save your direct links history as an m3u file";              type: "exportHistory" }
                ListElement { icon: "↧";  title: "Import History Playlist"; subtitle: "Load a modified history m3u file";                           type: "importHistory" }
                ListElement { icon: "🕒"; title: "Time Zone";               subtitle: "";                                                           type: "timezone" }
                ListElement { icon: "🎬"; title: "Auto Movie Poster";       subtitle: "Automatically fetch missing movie/series posters from OMDb"; type: "posterAuto" }
                ListElement { icon: "🔑"; title: "OMDb API Key";            subtitle: "Required for Auto Movie Poster — free key at omdbapi.com";   type: "omdbKey" }
                ListElement { icon: "▦";  title: "Grid View Columns";       subtitle: "Items per row in grid view folders and channels";            type: "gridColumns" }
                ListElement { icon: "▤";  title: "Poster View Columns";     subtitle: "Items per row in poster view";                               type: "posterColumns" }
                ListElement { icon: "⏱";  title: "Controller Timeout";      subtitle: "How long the player controls stay on screen before hiding";  type: "timeout" }
            }

            function activateRow(rowType) {
                if (rowType === "playlists") {
                    root.playlistsRequested();
                } else if (rowType === "updateAll") {
                    root.showToast("Updating playlists…");
                    AppController.refreshAllPlaylists();
                } else if (rowType === "backup") {
                    backupMenu.open();
                } else if (rowType === "exportHistory") {
                    exportHistoryDialog.openWithAutoName();
                } else if (rowType === "importHistory") {
                    importHistoryDialog.open();
                } else if (rowType === "timezone") {
                    timeZoneDialog.open();
                } else if (rowType === "posterAuto") {
                    posterAutoMenu.open();
                } else if (rowType === "omdbKey") {
                    omdbKeyField.text = AppController.settings.omdbApiKey;
                    omdbKeyMenu.open();
                }
            }

            delegate: Rectangle {
                id: row
                width: settingsList.width
                height: 72
                radius: 10
                color: "#1E1E1E"
                property bool isActiveItem: settingsList.activeFocus && ListView.isCurrentItem
                border.color: (rowHover.containsMouse || isActiveItem) ? "#FFD54F" : "#333333"
                border.width: isActiveItem ? 2 : 1

                readonly property bool hasInlineControl:
                    model.type === "gridColumns" || model.type === "posterColumns" || model.type === "timeout"

                Behavior on border.color { ColorAnimation { duration: 150 } }

                MouseArea {
                    id: rowHover
                    anchors.fill: parent
                    hoverEnabled: true
                    onClicked: {
                        settingsList.currentIndex = index;
                        if (!row.hasInlineControl) settingsList.activateRow(model.type);
                    }
                }

                Keys.onPressed: (event) => {
                    if (event.key === Qt.Key_Enter || event.key === Qt.Key_Return || event.key === Qt.Key_Space) {
                        if (!row.hasInlineControl) settingsList.activateRow(model.type);
                        event.accepted = true;
                    } else if (row.hasInlineControl
                               && (event.key === Qt.Key_Left || event.key === Qt.Key_Right)) {
                        // Adjust inline value with ←/→ while the row is focused
                        var dir = event.key === Qt.Key_Right ? 1 : -1;
                        if (model.type === "gridColumns") {
                            AppController.settings.gridColumns = nextColumnValue(AppController.settings.gridColumns, dir);
                        } else if (model.type === "posterColumns") {
                            AppController.settings.posterColumns = nextColumnValue(AppController.settings.posterColumns, dir);
                        } else if (model.type === "timeout") {
                            AppController.settings.controllerTimeout =
                                Math.min(10, Math.max(0.5, AppController.settings.controllerTimeout + dir * 0.5));
                        }
                        event.accepted = true;
                    }
                }

                // 0 (Auto) <-> 2..8
                function nextColumnValue(current, dir) {
                    var values = [0, 2, 3, 4, 5, 6, 7, 8];
                    var i = values.indexOf(current);
                    if (i < 0) i = 0;
                    i = Math.min(values.length - 1, Math.max(0, i + dir));
                    return values[i];
                }

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 16
                    anchors.rightMargin: 16
                    spacing: 14

                    Rectangle {
                        width: 42; height: 42
                        radius: 8
                        color: "#2A2A2A"
                        Text {
                            anchors.centerIn: parent
                            text: model.icon
                            color: "#FFD54F"
                            font.pixelSize: 20
                        }
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 2
                        Text {
                            text: model.title
                            color: "#FFFFFF"
                            font.pixelSize: 15
                            font.bold: true
                        }
                        Text {
                            text: model.type === "timezone"
                                  ? AppController.settings.timeZoneLabel
                                  : model.type === "posterAuto"
                                    ? (AppController.settings.autoPosterFetchEnabled ? "ON — fetching automatically" : "OFF")
                                    : model.type === "omdbKey"
                                      ? (AppController.settings.omdbApiKey.length > 0 ? "Key saved ✓" : model.subtitle)
                                      : model.subtitle
                            color: "#AAAAAA"
                            font.pixelSize: 12
                            elide: Text.ElideRight
                            Layout.fillWidth: true
                        }
                    }

                    // ----- Inline value controls -----
                    RowLayout {
                        visible: row.hasInlineControl
                        spacing: 8

                        Button {
                            visible: row.hasInlineControl
                            text: "−"
                            implicitWidth: 32; implicitHeight: 32
                            background: Rectangle {
                                color: parent.hovered ? "#333333" : "#2A2A2A"
                                radius: 6
                                border.color: "#444444"
                            }
                            contentItem: Text {
                                text: parent.text; color: "#FFD54F"; font.pixelSize: 16; font.bold: true
                                horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter
                            }
                            onClicked: {
                                settingsList.currentIndex = index;
                                if (model.type === "gridColumns") {
                                    AppController.settings.gridColumns = row.nextColumnValue(AppController.settings.gridColumns, -1);
                                } else if (model.type === "posterColumns") {
                                    AppController.settings.posterColumns = row.nextColumnValue(AppController.settings.posterColumns, -1);
                                } else {
                                    AppController.settings.controllerTimeout =
                                        Math.max(0.5, AppController.settings.controllerTimeout - 0.5);
                                }
                            }
                        }

                        Text {
                            color: "#FFFFFF"
                            font.pixelSize: 14
                            font.bold: true
                            horizontalAlignment: Text.AlignHCenter
                            Layout.preferredWidth: 60
                            text: {
                                if (model.type === "gridColumns") {
                                    return AppController.settings.gridColumns === 0
                                           ? "Auto" : AppController.settings.gridColumns;
                                } else if (model.type === "posterColumns") {
                                    return AppController.settings.posterColumns === 0
                                           ? "Auto" : AppController.settings.posterColumns;
                                }
                                return AppController.settings.controllerTimeout.toFixed(1) + "s";
                            }
                        }

                        Button {
                            visible: row.hasInlineControl
                            text: "+"
                            implicitWidth: 32; implicitHeight: 32
                            background: Rectangle {
                                color: parent.hovered ? "#333333" : "#2A2A2A"
                                radius: 6
                                border.color: "#444444"
                            }
                            contentItem: Text {
                                text: parent.text; color: "#FFD54F"; font.pixelSize: 16; font.bold: true
                                horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter
                            }
                            onClicked: {
                                settingsList.currentIndex = index;
                                if (model.type === "gridColumns") {
                                    AppController.settings.gridColumns = row.nextColumnValue(AppController.settings.gridColumns, 1);
                                } else if (model.type === "posterColumns") {
                                    AppController.settings.posterColumns = row.nextColumnValue(AppController.settings.posterColumns, 1);
                                } else {
                                    AppController.settings.controllerTimeout =
                                        Math.min(10, AppController.settings.controllerTimeout + 0.5);
                                }
                            }
                        }
                    }

                    Text {
                        visible: !row.hasInlineControl
                        text: "›"
                        color: "#666666"
                        font.pixelSize: 22
                    }
                }
            }
        }
    }

    // ===== Backup & Restore chooser =====
    Popup {
        id: backupMenu
        anchors.centerIn: parent
        width: 360
        modal: true
        focus: true
        background: Rectangle { color: "#1E1E1E"; radius: 12; border.color: "#333333" }

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 8
            spacing: 8

            Text {
                text: "Backup & Restore"
                color: "#FFD54F"
                font.pixelSize: 17
                font.bold: true
                Layout.margins: 8
            }

            Button {
                Layout.fillWidth: true
                text: "Backup app data to a JSON file"
                background: Rectangle { color: parent.hovered ? "#333333" : "#2A2A2A"; radius: 8 }
                contentItem: Text { text: parent.text; color: "#FFFFFF"; font.pixelSize: 14; padding: 12 }
                onClicked: { backupMenu.close(); exportBackupDialog.openWithAutoName(); }
            }

            Button {
                Layout.fillWidth: true
                text: "Restore app data from a JSON file"
                background: Rectangle { color: parent.hovered ? "#333333" : "#2A2A2A"; radius: 8 }
                contentItem: Text { text: parent.text; color: "#FFFFFF"; font.pixelSize: 14; padding: 12 }
                onClicked: { backupMenu.close(); importBackupDialog.open(); }
            }
            
            Button {
                Layout.fillWidth: true
                text: "Load Demo Data"
                background: Rectangle { color: parent.hovered ? "#333333" : "#2A2A2A"; radius: 8 }
                contentItem: Text { text: parent.text; color: "#FFFFFF"; font.pixelSize: 14; padding: 12 }
                onClicked: { backupMenu.close(); AppController.loadDemoData(); }
            }

            Text {
                text: "Restoring replaces all current playlists and settings."
                color: "#AAAAAA"
                font.pixelSize: 11
                Layout.margins: 8
            }
        }
    }

    // ===== Auto Movie Poster =====
    Popup {
        id: posterAutoMenu
        anchors.centerIn: parent
        width: 380
        modal: true
        focus: true
        background: Rectangle { color: "#1E1E1E"; radius: 12; border.color: "#333333" }

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 16
            spacing: 12

            Text {
                text: "Auto Movie Poster"
                color: "#FFD54F"
                font.pixelSize: 17
                font.bold: true
            }

            Text {
                text: "নতুন প্লেলিস্ট যোগ করলে, movie/series-এর মধ্যে যেগুলোর poster (tvg-logo) নেই, সেগুলোর জন্য OMDb থেকে অটো poster খুঁজে বসিয়ে দেওয়া হবে।"
                color: "#AAAAAA"
                font.pixelSize: 12
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }

            RowLayout {
                Layout.fillWidth: true
                Text {
                    text: "Auto-fetch on/off"
                    color: "#FFFFFF"
                    font.pixelSize: 14
                    Layout.fillWidth: true
                }
                Switch {
                    checked: AppController.settings.autoPosterFetchEnabled
                    onToggled: AppController.settings.autoPosterFetchEnabled = checked
                }
            }

            Text {
                visible: AppController.settings.omdbApiKey.length === 0
                text: "⚠️ আগে OMDb API Key বসান, তা না হলে poster fetch হবে না।"
                color: "#FFB74D"
                font.pixelSize: 11
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }

            Text {
                visible: AppController.posterFetcher.running
                text: AppController.posterFetcher.statusText
                color: "#4FC3F7"
                font.pixelSize: 12
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }

            Button {
                Layout.fillWidth: true
                enabled: AppController.settings.omdbApiKey.length > 0 && !AppController.posterFetcher.running
                text: AppController.posterFetcher.running ? "Fetching…" : "🖼️ Fetch Missing Posters Now"
                background: Rectangle { color: parent.hovered ? "#333333" : "#2A2A2A"; radius: 8 }
                contentItem: Text {
                    text: parent.text; color: parent.enabled ? "#FFFFFF" : "#777777"
                    font.pixelSize: 14; padding: 12
                    horizontalAlignment: Text.AlignHCenter
                }
                onClicked: {
                    root.showToast("Fetching missing posters from OMDb…");
                    AppController.fetchPostersForAllPlaylists();
                }
            }

            Button {
                Layout.fillWidth: true
                visible: AppController.posterFetcher.running
                text: "⏹️ Stop"
                background: Rectangle { color: parent.hovered ? "#442222" : "#331A1A"; radius: 8 }
                contentItem: Text { text: parent.text; color: "#FF8A80"; font.pixelSize: 14; padding: 12; horizontalAlignment: Text.AlignHCenter }
                onClicked: AppController.posterFetcher.stop()
            }
        }
    }

    // ===== OMDb API Key =====
    Popup {
        id: omdbKeyMenu
        anchors.centerIn: parent
        width: 380
        modal: true
        focus: true
        background: Rectangle { color: "#1E1E1E"; radius: 12; border.color: "#333333" }

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 16
            spacing: 10

            Text {
                text: "OMDb API Key"
                color: "#FFD54F"
                font.pixelSize: 17
                font.bold: true
            }

            Text {
                text: "<a href=\"https://www.omdbapi.com/apikey.aspx\" style=\"color: #4FC3F7;\">www.omdbapi.com/apikey.aspx</a> থেকে একটা ফ্রি key নিয়ে এখানে বসান।"
                color: "#AAAAAA"
                font.pixelSize: 12
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
                textFormat: Text.RichText
                onLinkActivated: function(link) { Qt.openUrlExternally(link) }
                
                MouseArea {
                    anchors.fill: parent
                    acceptedButtons: Qt.NoButton
                    cursorShape: parent.hoveredLink ? Qt.PointingHandCursor : Qt.ArrowCursor
                }
            }

            TextField {
                id: omdbKeyField
                Layout.fillWidth: true
                placeholderText: "OMDb API key"
                color: "#FFFFFF"
                background: Rectangle { color: "#2A2A2A"; radius: 6; border.color: "#444444" }
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 8
                Button {
                    Layout.fillWidth: true
                    text: "Save"
                    background: Rectangle { color: parent.hovered ? "#FFCA28" : "#FFD54F"; radius: 8 }
                    contentItem: Text { text: parent.text; color: "#121212"; font.pixelSize: 14; font.bold: true; padding: 12; horizontalAlignment: Text.AlignHCenter }
                    onClicked: {
                        AppController.settings.omdbApiKey = omdbKeyField.text;
                        omdbKeyMenu.close();
                        root.showToast("OMDb API key saved");
                    }
                }
                Button {
                    Layout.fillWidth: true
                    text: "Cancel"
                    background: Rectangle { color: parent.hovered ? "#333333" : "#2A2A2A"; radius: 8 }
                    contentItem: Text { text: parent.text; color: "#FFFFFF"; font.pixelSize: 14; padding: 12; horizontalAlignment: Text.AlignHCenter }
                    onClicked: omdbKeyMenu.close()
                }
            }
        }
    }

    Connections {
        target: AppController.posterFetcher
        function onBatchFinished(playlistId, found, total) {
            root.showToast("Poster fetch: " + found + "/" + total + " এ poster পাওয়া গেছে");
        }
    }

    // ===== File dialogs =====
    FileDialog {
        id: exportBackupDialog
        title: "Save backup"
        fileMode: FileDialog.SaveFile
        nameFilters: ["JSON files (*.json)"]
        defaultSuffix: "json"
        // Auto-fill name: app_name-date-time.json ("/" is not allowed in
        // Windows file names, so the date uses "-"), regenerated on every
        // open so the timestamp is current. selectedFile must be a FULL
        // path — the native dialog ignores a bare relative name.
        function openWithAutoName() {
            var stamp = Qt.formatDateTime(new Date(), "dd-MM-yy-hh_mm");
            var docs = StandardPaths.writableLocation(StandardPaths.DocumentsLocation);
            currentFolder = docs;
            selectedFile = docs + "/m3uVideoPlayerPc-" + stamp + ".json";
            open();
        }
        onAccepted: AppController.exportBackup(selectedFile)
    }

    FileDialog {
        id: importBackupDialog
        title: "Restore backup"
        fileMode: FileDialog.OpenFile
        nameFilters: ["JSON files (*.json)", "All files (*)"]
        onAccepted: AppController.importBackup(selectedFile)
    }

    FileDialog {
        id: exportHistoryDialog
        title: "Export history playlist"
        fileMode: FileDialog.SaveFile
        nameFilters: ["M3U files (*.m3u)"]
        defaultSuffix: "m3u"
        // Auto-name scheme: appName-History-dateTime.m3u
        // e.g. m3uVideoPlayer-History-18-07-26-10_34.m3u
        function openWithAutoName() {
            var stamp = Qt.formatDateTime(new Date(), "dd-MM-yy-hh_mm");
            var docs = StandardPaths.writableLocation(StandardPaths.DocumentsLocation);
            currentFolder = docs;
            selectedFile = docs + "/m3uVideoPlayer-History-" + stamp + ".m3u";
            open();
        }
        onAccepted: AppController.exportHistoryM3u(selectedFile)
    }

    FileDialog {
        id: importHistoryDialog
        title: "Import history playlist"
        fileMode: FileDialog.OpenFile
        nameFilters: ["M3U files (*.m3u *.m3u8)", "All files (*)"]
        onAccepted: AppController.importHistoryM3u(selectedFile)
    }

    TimeZoneDialog {
        id: timeZoneDialog
    }

    // ===== Toast =====
    Rectangle {
        id: toast
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 32
        width: toastText.implicitWidth + 40
        height: 44
        radius: 22
        color: "#CC1E1E1E"
        border.color: "#FFD54F"
        border.width: 1
        opacity: 0
        visible: opacity > 0
        z: 50

        Behavior on opacity { NumberAnimation { duration: 250 } }

        Text {
            id: toastText
            anchors.centerIn: parent
            color: "#FFFFFF"
            font.pixelSize: 13
        }

        Timer {
            id: toastTimer
            interval: 3500
            onTriggered: toast.opacity = 0
        }
    }
}
