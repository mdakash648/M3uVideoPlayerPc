import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Button {
    id: control
    property string iconName: ""
    
    height: 45
    
    background: Rectangle {
        color: control.hovered ? "#222222" : "transparent"
        radius: 8
        
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
            visible: control.hovered
        }
    }
    
    function getIconChar(name) {
        switch (name) {
            case "list": return "☰"
            case "link": return "∞"
            case "history": return "⟲"
            case "settings": return "⚙"
            default: return "▪"
        }
    }
    
    contentItem: RowLayout {
        spacing: 12
        
        Text {
            text: getIconChar(control.iconName)
            color: control.hovered ? "#FFFFFF" : "#AAAAAA"
            font.pixelSize: 18
            Layout.leftMargin: 10
            
            Behavior on color {
                ColorAnimation { duration: 150 }
            }
        }
        
        Text {
            text: control.text
            color: control.hovered ? "#FFFFFF" : "#AAAAAA"
            font.pixelSize: 16
            font.bold: control.hovered
            Layout.fillWidth: true
            
            Behavior on color {
                ColorAnimation { duration: 150 }
            }
        }
    }
}
