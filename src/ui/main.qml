import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
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
                stackView.push(channelViewComponent, { "groupId": id, "groupName": name })
            }
            onAllChannelsOpened: {
                stackView.push(channelViewComponent, { "groupId": -1, "groupName": "All Channels", "playlistId": playlistId })
            }
            onFavoritesOpened: {
                stackView.push(channelViewComponent, { "groupId": -1, "groupName": "Favorites", "favoritesMode": true })
            }
        }
    }

    Component {
        id: channelViewComponent
        ChannelListView {
            onBackRequested: stackView.pop()
            onChannelOpened: function(streamUrl, channelName) {
                stackView.push(playerViewComponent, { "streamUrl": streamUrl, "streamTitle": channelName })
            }
        }
    }

    Component {
        id: playerViewComponent
        PlayerView {
            onBackRequested: stackView.pop()
        }
    }

    Component {
        id: directLinkViewComponent
        DirectLinkView {}
    }
}
