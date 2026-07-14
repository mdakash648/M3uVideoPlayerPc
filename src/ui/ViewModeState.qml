pragma Singleton
import QtQuick

QtObject {
    // Shared across every ChannelListView instance (regular group, All Channels, Favorites)
    property string channelViewMode: "List"

    // Shared across GroupListView instances
    property string groupViewMode: "Grid"
}
