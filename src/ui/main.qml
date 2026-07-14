import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "components"
import "screens"

ApplicationWindow {
    id: mainWindow
    width: 1280
    height: 720
    visible: true
    title: qsTr("M3U Video Player Desktop")
    color: "#121212"

    RowLayout {
        anchors.fill: parent
        spacing: 0

        // Sidebar Navigation
        Sidebar {
            id: sidebar
            Layout.fillHeight: true
            Layout.preferredWidth: 250
            
            onNavigationRequested: (page) => {
                if (page === "Playlists") {
                    stackView.replace(playlistViewComponent)
                } else if (page === "DirectLink") {
                    stackView.replace(directLinkViewComponent)
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
            }
        }
    }

    // Components for StackView
    Component {
        id: playlistViewComponent
        PlaylistListView {}
    }

    Component {
        id: directLinkViewComponent
        DirectLinkView {}
    }
}
