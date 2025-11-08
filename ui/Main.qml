import QtQuick
import QtQuick.Effects

//UI declarations

Window { //Root app window
    visible: true
    width: 1920
    height: 1080
    color: "#000000"
    title: qsTr("PCM Makerspace Checkin")
    visibility: Window.FullScreen //Make fullscreen


    enum AppState {
        Idle,
        Register,
        Welcome
    }

    property int appstate: Main.AppState.Idle; //Track app state
    property int transitionDuration: 1000;

    Item { //Container
        anchors.fill: parent;

        states : [ //Define the states for the application
            State {
                name: "idleState"
                when: appstate == Main.AppState.Idle //Activate state when appstate is set to idle
                PropertyChanges { //Set which properties should change when state is activated
                    target: idleFrame
                    opacity: 1
                }
                PropertyChanges {
                    target : registerFrame
                    opacity: 0
                }
                PropertyChanges {
                    target : welcomeFrame
                    opacity: 0
                }
                PropertyChanges {
                    target: pcmLogo
                    x: (Screen.width - pcmLogo.width) / 2
                    y: 90
                    scale: 1
                }
            },
            State {
                name: "qrState"
                when: appstate == Main.AppState.Register;
                PropertyChanges {
                    target: idleFrame
                    opacity: 0
                }
                PropertyChanges {
                    target : registerFrame
                    opacity: 1
                }
                PropertyChanges {
                    target : welcomeFrame
                    opacity: 0
                }
                PropertyChanges {
                    target: pcmLogo
                    scale: 0.7
                    x: -20
                    y: (Screen.height - pcmLogo.height)/2

                }
            },
            State {
                name: "welcomeState"
                when: appstate == Main.AppState.Welcome;
                PropertyChanges {
                    target: idleFrame
                    opacity: 0
                }
                PropertyChanges {
                    target : registerFrame
                    opacity: 0
                }
                PropertyChanges {
                    target : welcomeFrame
                    opacity: 1
                }
                PropertyChanges {
                    target: pcmLogo
                    x: (Screen.width - pcmLogo.width) / 2
                    y: 90
                    scale : 1
                }
            }
        ]

        transitions : [ //Animate transitions between states
            Transition {
                from: "qrState"; to: "idleState"
                SequentialAnimation { //Animations!!
                    ScriptAction { script: idleFrame.visible = true }
                    ParallelAnimation {
                        NumberAnimation { //Animate numerical value from whatever it is currently to the target value set in the state's propertyChanges
                            target: pcmLogo
                            property: "x"
                            duration: transitionDuration/2.0
                            easing.type: Easing.InOutQuad
                        }
                        NumberAnimation {
                            target: pcmLogo
                            property: "y"
                            duration: transitionDuration/2.0
                            easing.type: Easing.InOutQuad
                        }
                        NumberAnimation {
                            target: registerFrame
                            property: "opacity"
                            duration: transitionDuration/2.5
                        }
                    }
                    ParallelAnimation {
                        NumberAnimation {
                            target: pcmLogo
                            property: "scale"
                            duration: transitionDuration/2.0
                            easing.type: Easing.InOutQuad
                        }
                        NumberAnimation {
                            target: idleFrame
                            property: "opacity"
                            duration: transitionDuration/2.5
                        }
                    }
                    ScriptAction { script: registerFrame.visible = false }
                }
            },
            Transition {
                from: "idleState"; to: "welcomeState"
                SequentialAnimation {
                    ScriptAction { script: welcomeFrame.visible = true }
                    NumberAnimation {
                        target: idleFrame
                        property: "opacity"
                        duration: transitionDuration/2.5
                    }
                    NumberAnimation {
                        target: welcomeFrame
                        property: "opacity"
                        duration: transitionDuration/2.5
                    }
                    ScriptAction { script: idleFrame.visible = false }
                }
            },
            Transition {
                from: "welcomeState"; to: "idleState"
                SequentialAnimation {
                    ScriptAction { script: idleFrame.visible = true }
                    NumberAnimation {
                        target: welcomeFrame
                        property: "opacity"
                        duration: transitionDuration/2.5
                    }
                    NumberAnimation {
                        target: idleFrame
                        property: "opacity"
                        duration: transitionDuration/2.5
                    }
                    ScriptAction { script: welcomeFrame.visible = false }
                }
            },
            Transition {
                from: "idleState"; to: "qrState"
                SequentialAnimation {
                    ScriptAction { script: registerFrame.visible = true }
                    ParallelAnimation {
                        NumberAnimation {
                            target: idleFrame
                            property: "opacity"
                            duration: transitionDuration/2.5
                        }
                        NumberAnimation {
                            target: pcmLogo
                            property: "scale"
                            duration: transitionDuration/2.0
                            easing.type: Easing.InOutQuad
                        }
                    }
                    ParallelAnimation {
                        NumberAnimation { //Animate qrcode scale from 0 to 1
                            target: qrCode
                            property: "scale"
                            duration: transitionDuration/2.0
                            from: 0.0
                            to: 1.0
                        }

                        NumberAnimation {
                            target: registerFrame
                            property: "opacity"
                            duration: transitionDuration/2.5
                        }
                        NumberAnimation {
                            target: pcmLogo
                            property: "x"
                            duration: transitionDuration/2.0
                            easing.type: Easing.InOutQuad
                        }
                        NumberAnimation {
                            target: pcmLogo
                            property: "y"
                            duration: transitionDuration/2.0
                            easing.type: Easing.InOutQuad
                        }
                    }
                    ScriptAction { script: idleFrame.visible = false }
                }
            }
        ]

        Item { //idleFrame container
            id: idleFrame
            visible: true
            opacity: 1
            anchors.fill: parent

            Image {
                id: ltx2a
                source: "../resources/ltx2a_ledoff.svg" //image source relative to Main.qml (image files must be explicitly included in CMakeLists.txt)
                height: 300
                fillMode: Image.PreserveAspectFit
                anchors.left: parent.left
                anchors.leftMargin: parent.width * 0.25
                anchors.top: parent.top
                anchors.topMargin: parent.height * 0.05 + 390
            }

            Image {
                id: card
                source: "../resources/card_filled.svg"
                height: 150
                fillMode: Image.PreserveAspectFit
                anchors.right: parent.right
                anchors.rightMargin: parent.width * (0.35 + offset)
                anchors.top: parent.top
                anchors.topMargin: parent.height * 0.05 + 120 + 390
                property real offset: 0

                SequentialAnimation on offset { //Animate the card moving and LTx2A lighting up
                    loops: Animation.Infinite
                    NumberAnimation { //Animate the offset (and therefore the horizontal position of card)
                        from: 0;
                        to: 0.175;
                        duration: 1000;
                        easing.type: Easing.InOutCubic;
                        easing.amplitude: 1;
                        easing.period: 1.0;
                    }
                    ScriptAction {
                        script: ltx2a.source = "../resources/ltx2a_ledon.svg" //Change image of LTx2A
                    }
                    NumberAnimation { //Do nothing for 1 seconds
                        from: 0.175;
                        to : 0.175;
                        duration: 1000;
                    }
                    ScriptAction {
                        script: ltx2a.source = "../resources/ltx2a_ledoff.svg"
                    }
                    NumberAnimation { //Animate card back to starting pos
                        from: 0.175;
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
            id: welcomeFrame
            anchors.fill: parent
            visible: false
            opacity: 0

            Text {
                id: welcomeText
                text: "Welcome back, member!" //In the future this will be set to user name based on airtable data
                font.pointSize: 100
                anchors.horizontalCenter: parent.horizontalCenter;
                anchors.verticalCenter: parent.verticalCenter;
                color: "#ffffff"
            }
        }


        Item {
            id: registerFrame
            anchors.fill: parent
            visible: false
            opacity: 0

            Image {
                id : qrCode
                width: 600
                height: 600
                anchors.right: parent.right
                anchors.rightMargin: 175
                anchors.top: parent.top
                anchors.topMargin: 100
                fillMode: Image.PreserveAspectFit
                source : "image://qr/generated" //dynamically get qr code from QRImageProvider (see main.cpp)
                Connections { //Connect QRManager's onQrCodeChanged event to a function in QML
                    target : QRManager
                    function onQrCodeChanged() {
                        qrCode.source = "image://qr/generated?refresh=" + Math.random() //Change the source url from the dynamic image to regenerate and redraw it
                    }
                }
            }

            Text {
                id: registerText
                text: "Scan QR Code to Register"
                font.pointSize: 50
                anchors.bottom: parent.bottom
                anchors.bottomMargin: parent.height * 0.05
                anchors.horizontalCenter: parent.horizontalCenter
                color: "#ffffff"
            }
        }

        Image {
            id : pcmLogo
            source: "../resources/PC-LogoText.svg"
            height: 300
            sourceSize: Qt.size(409, 510)
            fillMode: Image.PreserveAspectFit
            x: (Screen.width - pcmLogo.width) / 2
            y: 90
            visible: true
            opacity: 1

            MultiEffect { //Rainbow effect
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
            }
        }
    }
}

