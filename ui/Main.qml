import QtQuick
import QtQuick.Effects
import QtQuick.Controls
import QtQuick.Controls.Material

//UI declarations

Window { //Root app window
    visible: true
    width: 800
    height: 600
    color: "#871C1C"
    title: qsTr("PCM 3D Printing Kiosk")
    //visibility: Window.FullScreen //Make fullscreen

    enum AppState {
        Idle,
        Message,
        Scan,
        Printing
    }

    Material.theme : Material.Dark

    property int appstate: Main.AppState.Idle; //Track app state
    property int transitionDuration: 1000;

    Item { //Container
        anchors.fill: parent;

        states: [
            State {
                name: "idleState"
                when: appstate == Main.AppState.Idle
                PropertyChanges {
                    target: idleFrame
                    opacity: 1
                    visible: true
                }
                PropertyChanges {
                    target: scanFrame
                    opacity: 0
                    visible: false
                }
                PropertyChanges {
                    target: messageFrame
                    opacity: 0
                }
                PropertyChanges {
                    target: printingFrame
                    opacity: 0
                }
            },
            State {
                name: "scanState"
                when: appstate == Main.AppState.Scan
                PropertyChanges {
                    target: idleFrame
                    opacity: 0
                    visible: false
                }
                PropertyChanges {
                    target: scanFrame
                    opacity: 1
                    visible: true
                }
                PropertyChanges {
                    target: messageFrame
                    opacity: 0
                }
                PropertyChanges {
                    target: printingFrame
                    opacity: 0
                }
            }

        ]

        Image {
            id : pcmLogo
            source: "../resources/PC-LogoText.svg"
            height: 100
            sourceSize: Qt.size(409, 510)
            fillMode: Image.PreserveAspectFit
            anchors.left: parent.left
            anchors.top: parent.top
            anchors.topMargin: 25
            anchors.leftMargin: 25
            visible: true
            opacity: 1

            /*MultiEffect { //Rainbow effect
                anchors.fill: pcmLogo
                source: pcmLogo
                colorization: 1.0 //Color overlay (on the white svg)
                colorizationColor: Qt.hsva(hueValue, 1.0, 1.0, 1.0)

                property real hueValue: 0

                SequentialAnimation on hueValue { //Infinite rainbow color changing animation
                    loops: Animation.Infinite
                    NumberAnimation {
                        from: 0
                        to: 1
                        duration: 256000 //Takes 256 seconds per loop
                    }
                }
            }*/
        }

        Item { //idleFrame container
            id: idleFrame
            visible: true
            opacity: 1
            anchors.fill: parent

            Text {
                id: idleText
                anchors.centerIn: parent
                font.pointSize: 36
                color: "#fff"
                text: "Welcome to the\nPhysical Computing Makerspace!"
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }

            Item {

                anchors.bottom: parent.bottom
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.bottomMargin: 100
                width: 420
                height: 50

                RoundButton {
                    id: orcaSlicerButton
                    anchors.bottom: parent.bottom
                    anchors.right: parent.right
                    onClicked: backend.orcaButtonClicked();
                    width: 200
                    height: 50
                    radius: 5
                    background: Rectangle {
                        color: orcaSlicerButton.down ? "#6b1616" : "#871C1C"
                        border.width: 1
                        border.color: "#fff"
                        radius: 5
                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            acceptedButtons: Qt.NoButton
                            hoverEnabled: true
                        }
                    }

                    Item {
                        anchors.centerIn: parent
                        anchors.fill: parent
                        Image {
                            id: orcaSlicerImage
                            source: "../resources/OrcaSlicer.svg"
                            height: 24
                            width: 24
                            fillMode: Image.PreserveAspectFit
                            anchors.verticalCenter: parent.verticalCenter
                            anchors.left: parent.left
                            anchors.leftMargin: 40
                        }

                        Text {
                            text: "Open OrcaSlicer"
                            color: "#fff"
                            anchors.left: orcaSlicerImage.right
                            anchors.leftMargin: 5
                            anchors.verticalCenter: parent.verticalCenter
                        }
                    }
                }

                RoundButton {
                    id: helpButton
                    anchors.bottom: parent.bottom
                    anchors.left: parent.left
                    onClicked: backend.helpButtonClicked();
                    width: 200
                    height: 50
                    radius: 5
                    background: Rectangle {
                        color: helpButton.down ? "#6b1616" : "#871C1C"
                        border.width: 1
                        border.color: "#fff"
                        radius: 5
                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            acceptedButtons: Qt.NoButton
                            hoverEnabled: true
                        }
                    }
                    Item {
                        anchors.centerIn: parent
                        anchors.fill: parent
                        Image {
                            id: helpImage
                            source: "../resources/help.svg"
                            height: 24
                            width: 24
                            fillMode: Image.PreserveAspectFit
                            anchors.verticalCenter: parent.verticalCenter
                            anchors.left: parent.left
                            anchors.leftMargin: 70
                        }

                        Text {
                            text: "Help"
                            color: "#fff"
                            anchors.left: helpImage.right
                            anchors.leftMargin: 5
                            anchors.verticalCenter: parent.verticalCenter
                        }
                    }
                }
            }
        }


        Item { //scanFrame container
            id: scanFrame
            visible: false
            opacity: 0
            anchors.fill: parent

            Image {
                id: ltx2a
                source: "../resources/ltx2a_ledoff.svg" //image source relative to Main.qml (image files must be explicitly included in CMakeLists.txt)
                height: 300
                fillMode: Image.PreserveAspectFit
                anchors.left: parent.left
                anchors.leftMargin: 100
                anchors.top: parent.top
                anchors.topMargin: 165
            }

            Image {
                id: card
                source: "../resources/card_filled.svg"
                height: 150
                fillMode: Image.PreserveAspectFit
                anchors.right: parent.right
                anchors.rightMargin: 150 + 235*offset
                anchors.top: parent.top
                anchors.topMargin: 260
                property real offset: 0

                SequentialAnimation on offset { //Animate the card moving and LTx2A lighting up
                    loops: Animation.Infinite
                    NumberAnimation { //Animate the offset (and therefore the horizontal position of card)
                        from: 0;
                        to: 1;
                        duration: 1000;
                        easing.type: Easing.InOutCubic;
                        easing.amplitude: 1;
                        easing.period: 1.0;
                    }
                    ScriptAction {
                        script: ltx2a.source = "../resources/ltx2a_ledon.svg" //Change image of LTx2A
                    }
                    NumberAnimation { //Do nothing for 1 seconds
                        from: 1;
                        to : 1;
                        duration: 1000;
                    }
                    ScriptAction {
                        script: ltx2a.source = "../resources/ltx2a_ledoff.svg"
                    }
                    NumberAnimation { //Animate card back to starting pos
                        from: 1;
                        to: 0;
                        duration: 1000;
                        easing.type: Easing.InOutCubic
                        easing.amplitude: 1;
                        easing.period: 1.0;
                    }
                    NumberAnimation { //Do nothing for 2 seconds
                        from: 0;
                        to : 0;
                        duration: 2000;
                    }
                }
            }

            Text {
                id: tapText
                text: "Tap your ID or UCard"
                font.pointSize: 50
                anchors.bottom: parent.bottom
                anchors.bottomMargin: parent.height * 0.05
                anchors.horizontalCenter: parent.horizontalCenter
                color: "#ffffff"
            }
        }

        Item {
            id: printingFrame
            anchors.fill: parent
            visible: false
            opacity: 0

            Text {
                id: printingText
                text: "Printing now!" //In the future this will be set to user name based on airtable data
                font.pointSize: 100
                anchors.horizontalCenter: parent.horizontalCenter;
                anchors.verticalCenter: parent.verticalCenter;
                color: "#ffffff"
            }
        }

        Item {
            id: messageFrame
            anchors.fill: parent
            visible: false
            opacity: 0

            Text {
                id: messageText
                text: "OOOH FOrm goes here" //In the future this will be set to user name based on airtable data
                font.pointSize: 100
                anchors.horizontalCenter: parent.horizontalCenter;
                anchors.verticalCenter: parent.verticalCenter;
                color: "#ffffff"
            }
        }


    }
}


// states : [ //Define the states for the application
//     State {
//         name: "idleState"
//         when: appstate == Main.AppState.Idle //Activate state when appstate is set to idle
//         PropertyChanges { //Set which properties should change when state is activated
//             target: idleFrame
//             opacity: 1
//         }
//         PropertyChanges {
//             target : registerFrame
//             opacity: 0
//         }
//         PropertyChanges {
//             target : welcomeFrame
//             opacity: 0
//         }
//         PropertyChanges {
//             target: pcmLogo
//             x: (Screen.width - pcmLogo.width) / 2
//             y: 90
//             scale: 1
//         }
//     },
//     State {
//         name: "qrState"
//         when: appstate == Main.AppState.Register;
//         PropertyChanges {
//             target: idleFrame
//             opacity: 0
//         }
//         PropertyChanges {
//             target : registerFrame
//             opacity: 1
//         }
//         PropertyChanges {
//             target : welcomeFrame
//             opacity: 0
//         }
//         PropertyChanges {
//             target: pcmLogo
//             scale: 0.7
//             x: -20
//             y: (Screen.height - pcmLogo.height)/2

//         }
//     },
//     State {
//         name: "welcomeState"
//         when: appstate == Main.AppState.Welcome;
//         PropertyChanges {
//             target: idleFrame
//             opacity: 0
//         }
//         PropertyChanges {
//             target : registerFrame
//             opacity: 0
//         }
//         PropertyChanges {
//             target : welcomeFrame
//             opacity: 1
//         }
//         PropertyChanges {
//             target: pcmLogo
//             x: (Screen.width - pcmLogo.width) / 2
//             y: 90
//             scale : 1
//         }
//     }
// ]

// transitions : [ //Animate transitions between states
//     Transition {
//         from: "qrState"; to: "idleState"
//         SequentialAnimation { //Animations!!
//             ScriptAction { script: idleFrame.visible = true }
//             ParallelAnimation {
//                 NumberAnimation { //Animate numerical value from whatever it is currently to the target value set in the state's propertyChanges
//                     target: pcmLogo
//                     property: "x"
//                     duration: transitionDuration/2.0
//                     easing.type: Easing.InOutQuad
//                 }
//                 NumberAnimation {
//                     target: pcmLogo
//                     property: "y"
//                     duration: transitionDuration/2.0
//                     easing.type: Easing.InOutQuad
//                 }
//                 NumberAnimation {
//                     target: registerFrame
//                     property: "opacity"
//                     duration: transitionDuration/2.5
//                 }
//             }
//             ParallelAnimation {
//                 NumberAnimation {
//                     target: pcmLogo
//                     property: "scale"
//                     duration: transitionDuration/2.0
//                     easing.type: Easing.InOutQuad
//                 }
//                 NumberAnimation {
//                     target: idleFrame
//                     property: "opacity"
//                     duration: transitionDuration/2.5
//                 }
//             }
//             ScriptAction { script: registerFrame.visible = false }
//         }
//     },
//     Transition {
//         from: "idleState"; to: "welcomeState"
//         SequentialAnimation {
//             ScriptAction { script: welcomeFrame.visible = true }
//             NumberAnimation {
//                 target: idleFrame
//                 property: "opacity"
//                 duration: transitionDuration/2.5
//             }
//             NumberAnimation {
//                 target: welcomeFrame
//                 property: "opacity"
//                 duration: transitionDuration/2.5
//             }
//             ScriptAction { script: idleFrame.visible = false }
//         }
//     },
//     Transition {
//         from: "welcomeState"; to: "idleState"
//         SequentialAnimation {
//             ScriptAction { script: idleFrame.visible = true }
//             NumberAnimation {
//                 target: welcomeFrame
//                 property: "opacity"
//                 duration: transitionDuration/2.5
//             }
//             NumberAnimation {
//                 target: idleFrame
//                 property: "opacity"
//                 duration: transitionDuration/2.5
//             }
//             ScriptAction { script: welcomeFrame.visible = false }
//         }
//     },
//     Transition {
//         from: "idleState"; to: "qrState"
//         SequentialAnimation {
//             ScriptAction { script: registerFrame.visible = true }
//             ParallelAnimation {
//                 NumberAnimation {
//                     target: idleFrame
//                     property: "opacity"
//                     duration: transitionDuration/2.5
//                 }
//                 NumberAnimation {
//                     target: pcmLogo
//                     property: "scale"
//                     duration: transitionDuration/2.0
//                     easing.type: Easing.InOutQuad
//                 }
//             }
//             ParallelAnimation {
//                 NumberAnimation { //Animate qrcode scale from 0 to 1
//                     target: qrCode
//                     property: "scale"
//                     duration: transitionDuration/2.0
//                     from: 0.0
//                     to: 1.0
//                 }

//                 NumberAnimation {
//                     target: registerFrame
//                     property: "opacity"
//                     duration: transitionDuration/2.5
//                 }
//                 NumberAnimation {
//                     target: pcmLogo
//                     property: "x"
//                     duration: transitionDuration/2.0
//                     easing.type: Easing.InOutQuad
//                 }
//                 NumberAnimation {
//                     target: pcmLogo
//                     property: "y"
//                     duration: transitionDuration/2.0
//                     easing.type: Easing.InOutQuad
//                 }
//             }
//             ScriptAction { script: idleFrame.visible = false }
//         }
//     }
// ]
