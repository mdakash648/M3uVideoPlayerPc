import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Button {
    id: control
    property string iconName: ""
    
    height: 45
    
    background: Rectangle {
        color: control.hovered ? "#2A2A2A" : "transparent"
        radius: 8
        
        Behavior on color {
            ColorAnimation { duration: 150 }
        }
    }
    
    contentItem: RowLayout {
        spacing: 12
        
        // Placeholder for icon (can be replaced with FontAwesome or image later)
        Rectangle {
            width: 20
            height: 20
            color: control.down ? "#FFD54F" : (control.hovered ? "#FFFFFF" : "#AAAAAA")
            radius: 4
            Layout.leftMargin: 10
        }
        
        Text {
            text: control.text
            color: control.down ? "#FFD54F" : (control.hovered ? "#FFFFFF" : "#CCCCCC")
            font.pixelSize: 16
            font.bold: control.hovered
            Layout.fillWidth: true
            
            Behavior on color {
                ColorAnimation { duration: 150 }
            }
        }
    }
}
