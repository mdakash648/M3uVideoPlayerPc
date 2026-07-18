import QtQuick
import QtQuick.Controls

Button {
    id: root

    property int currentSort: 2 // 0 = Ascending, 1 = Descending, 2 = Playlist Order
    signal sortSelected(int mode)

    text: "Sort: " + labelFor(currentSort)

    function labelFor(mode) {
        if (mode === 0) return "A-Z"
        if (mode === 1) return "Z-A"
        return "Playlist"
    }

    background: Rectangle { 
        color: "#1E1E1E"
        radius: 6
        implicitHeight: 40
        implicitWidth: 130
        border.color: root.activeFocus ? "#FFD54F" : "#333333"
        border.width: root.activeFocus ? 2 : 1
    }
    contentItem: Text {
        text: root.text
        color: "#FFD54F"
        font.pixelSize: 13
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
    }

    onClicked: sortMenu.open()

    Keys.onReturnPressed: root.clicked()
    Keys.onEnterPressed: root.clicked()

    Menu {
        id: sortMenu
        y: root.height

        MenuItem {
            text: "Ascending (A-Z)"
            onTriggered: root.sortSelected(0)
        }
        MenuItem {
            text: "Descending (Z-A)"
            onTriggered: root.sortSelected(1)
        }
        MenuItem {
            text: "According to Playlist"
            onTriggered: root.sortSelected(2)
        }
    }
}
