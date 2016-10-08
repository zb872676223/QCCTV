/*
 * Copyright (c) 2016 Alex Spataru
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE
 */

import QtQuick 2.0
import QtMultimedia 5.4
import QtQuick.Controls 2.0

import "."

ApplicationWindow {
    id: app
    
    //
    // Window geometry
    //
    width: 720
    height: 480
    
    //
    // Window properties
    //
    color: "#000"
    visible: true
    title: AppDspName + " " + AppVersion

    //
    // Global variables
    //
    property var borderSize: 8
    property string family: "OpenSans"
    property bool controlsEnabled: true

    //
    // QCCTV signals/slots
    //
    Connections {
        target: QCCTVCamera
        onFpsChanged: fps.text = QCCTVCamera.fps() + " FPS"
        onCameraNameChanged: camName.text = QCCTVCamera.cameraName()
        onFocusStatusChanged: status.display (qsTr ("Focusing Camera") + "...")
    }

    //
    // Video output
    //
    VideoOutput {
        id: video
        //color: "transparent"
        anchors.fill: parent

        source: Camera {}

        //
        // Control toggler mouse area
        //
        MouseArea {
            anchors.fill: parent
            onClicked: app.controlsEnabled = !app.controlsEnabled
        }
    }

    //
    // Top status bar
    //
    Rectangle {
        id: menu
        
        //
        // Geometry specifications
        //
        radius: 2
        border.width: 1
        height: 24 + borderSize

        //
        // Allow hiding/showing this control
        //
        opacity: app.controlsEnabled ? 0.85 : 0
        Behavior on opacity { NumberAnimation{} }

        //
        // Colors
        //
        color: "#444"
        border.color: "#999"

        //
        // Layout options
        //
        anchors {
            top: parent.top
            left: parent.left
            right: parent.right
            margins: borderSize * 2
        }

        //
        // Camera name
        //
        Label {
            id: camName
            color: "#fff"
            font.family: app.family
            text: QCCTVCamera.cameraName()

            anchors {
                left: parent.left
                margins: borderSize
                verticalCenter: parent.verticalCenter
            }
        }

        //
        // FPS indicator
        //
        Label {
            id: fps
            color: "white"
            font.family: app.family
            text: QCCTVCamera.fps() + " FPS"

            anchors {
                right: parent.right
                margins: borderSize
                verticalCenter: parent.verticalCenter
            }
        }
    }

    //
    // Action buttons
    //
    Row {
        spacing: borderSize * 2

        //
        // Allow hiding/showing these controls
        //
        opacity: app.controlsEnabled ? 1 : 0
        Behavior on opacity { NumberAnimation{} }

        //
        // Layout options
        //
        anchors {
            bottom: parent.bottom
            margins: borderSize * 2
            horizontalCenter: parent.horizontalCenter
        }

        //
        // Light control button
        //
        Button {
            width: 54
            height: 54
            source: "qrc:/images/light.png"
            anchors.verticalCenter: parent.verticalCenter

            onClicked: {
                toggled = !toggled

                if (toggled) {
                    QCCTVCamera.turnOnLight()
                    status.display (qsTr ("Flashlight Enabled"))
                }

                else {
                    QCCTVCamera.turnOffLight()
                    status.display (qsTr ("Flashlight Disabled"))
                }
            }
        }

        //
        // Settings button
        //
        Button {
            width: 64
            height: 64
            source: "qrc:/images/settings.png"
            anchors.verticalCenter: parent.verticalCenter
        }

        //
        // Photo button
        //
        Button {
            width: 64
            height: 64
            source: "qrc:/images/recorder.png"
            anchors.verticalCenter: parent.verticalCenter
        }

        //
        // Focus button
        //
        Button {
            width: 54
            height: 54
            source: "qrc:/images/focus.png"
            onClicked: QCCTVCamera.focusCamera()
            anchors.verticalCenter: parent.verticalCenter
        }
    }

    //
    // Status label
    //
    Rectangle {
        id: status

        //
        // Geometry options
        //
        radius: 2
        height: 24
        border.width: 1
        width: Math.min (app.width * 0.6, sText.width * 2)

        //
        // Colors
        //
        color: "#444"
        border.color: "#999"
        
        //
        // Allow hiding/showing this control
        //
        opacity: 0
        Behavior on opacity { NumberAnimation{} }
        
        //
        // Layout options
        //
        anchors {
            top: parent.top
            topMargin: menu.height + 4 * borderSize
            horizontalCenter: video.horizontalCenter
        }

        //
        // Shows the status window with the given \a text
        // for two seconds and then hides the window
        //
        function display (text) {
            sTimer.restart()
            sText.text = text
            status.opacity = 0.85
        }

        //
        // Window hide timer
        //
        Timer {
            id: sTimer
            interval: 2000
            onTriggered: status.opacity = 0
        }

        //
        // Dynamic status label
        //
        Label {
            id: sText
            color: "#fff"
            font.family: app.family
            anchors.centerIn: parent
        }
    }
}
