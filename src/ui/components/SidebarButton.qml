import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Button {
    id: control
    property string iconName: ""
    property bool isCollapsed: false

    height: 45

    padding: 0

    // QtQuick Button fires on Space by default; make Enter/Return work too.
    Keys.onReturnPressed: control.clicked()
    Keys.onEnterPressed: control.clicked()
    
    background: Rectangle {
        color: (control.hovered || control.activeFocus) ? "#222222" : "transparent"
        radius: 8
        border.color: control.activeFocus ? "#FFD54F" : "transparent"
        border.width: control.activeFocus ? 1 : 0
        
        Behavior on color {
            ColorAnimation { duration: 150 }
        }
        
        // Right side 3px border bonus touch
        Rectangle {
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            anchors.topMargin: 6
            anchors.bottomMargin: 6
            width: 3
            color: "#FFD54F"
            radius: 2
            visible: control.hovered || control.activeFocus
        }
    }
    
    function getIconChar(name) {
        switch (name) {
            case "list": return "☰"
            case "link": return "∞"
            case "settings": return "⚙"
            default: return "▪"
        }
    }
    
    contentItem: Item {
        Text {
            id: iconText
            text: getIconChar(control.iconName)
            color: (control.hovered || control.activeFocus) ? "#FFFFFF" : "#AAAAAA"
            font.pixelSize: 18
            anchors.verticalCenter: parent.verticalCenter
            x: control.isCollapsed ? (parent.width - width) / 2 : 10
            
            Behavior on color {
                ColorAnimation { duration: 150 }
            }
            
            Behavior on x {
                NumberAnimation { duration: 200; easing.type: Easing.InOutQuad }
            }
        }
        
        Text {
            visible: !control.isCollapsed
            text: control.text
            color: (control.hovered || control.activeFocus) ? "#FFFFFF" : "#AAAAAA"
            font.pixelSize: 16
            font.bold: control.hovered || control.activeFocus
            anchors.verticalCenter: parent.verticalCenter
            anchors.left: iconText.right
            anchors.leftMargin: 12
            anchors.right: parent.right
            elide: Text.ElideRight
            
            Behavior on color {
                ColorAnimation { duration: 150 }
            }
        }
    }
}
