import QtQuick
import QtQuick.Effects
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Dialogs

//UI declarations

Window { //Root app window
    id: rootWindow
    visible: true
    width: 800
    height: 600
    color: "#871C1C"
    title: qsTr("PCM 3D Printing Kiosk")
    //visibility: Window.FullScreen //Make fullscreen

    enum AppState {
        Idle,
        Prep,
        Message,
        UserScan,
        StaffScan,
        Printing,
        Loading
    }

    Timer {
        id: stateTimeoutTimer
        interval: 120000
        repeat: false
        onTriggered: {
            rootWindow.appstate = Main.AppState.Idle
        }
    }

    onAppstateChanged: {
        switch (rootWindow.appstate) {
        case Main.AppState.Prep:
        case Main.AppState.Printing:
        case Main.AppState.UserScan:
        case Main.AppState.StaffScan:
        case Main.AppState.Loading:
            stateTimeoutTimer.restart()
            break

        case Main.AppState.Idle:
            stateTimeoutTimer.stop()
            break

        case Main.AppState.Message:
            if (messageAcceptText === "Training Completed") {
                stateTimeoutTimer.stop()
                break;
            } else {
                stateTimeoutTimer.restart()
                break;
            }

        default:
            stateTimeoutTimer.stop()
            break
        }
    }

    Material.theme : Material.Dark

    property int appstate: Main.AppState.Idle; //Track app state
    property int transitionDuration: 1000;

    Item { //Container
        id: rootItem
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
                PropertyChanges {
                    target: prepFrame
                    opacity: 0
                }
                PropertyChanges {
                    target: loadingFrame
                    opacity: 0
                }
            },
            State {
                name: "scanState"
                when: appstate == Main.AppState.UserScan || appstate == Main.AppState.StaffScan
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
            },
            State {
                name: "prepState"
                when: appstate == Main.AppState.Prep
                PropertyChanges {
                    target: idleFrame
                    opacity: 0
                    visible: false
                }
                PropertyChanges {
                    target: scanFrame
                    opacity: 0
                    visible: false
                }
                PropertyChanges {
                    target: messageFrame
                    opacity: 0
                    visible: false
                }
                PropertyChanges {
                    target: printingFrame
                    opacity: 0
                    visible: false
                }
                PropertyChanges {
                    target: prepFrame
                    opacity: 1
                    visible: true
                }
                PropertyChanges {
                    target: loadingFrame
                    opacity: 0
                    visible: false
                }
            },
            State {
                name: "messageState"
                when: appstate == Main.AppState.Message
                PropertyChanges {
                    target: idleFrame
                    opacity: 0
                    visible: false
                }
                PropertyChanges {
                    target: scanFrame
                    opacity: 0
                    visible: false
                }
                PropertyChanges {
                    target: messageFrame
                    opacity: 1
                    visible: true
                }
                PropertyChanges {
                    target: printingFrame
                    opacity: 0
                    visible: false
                }
                PropertyChanges {
                    target: prepFrame
                    opacity: 0
                    visible: false
                }
                PropertyChanges {
                    target: loadingFrame
                    opacity: 0
                    visible: false
                }
            },
            State {
                name: "loadingState"
                when: appstate == Main.AppState.Loading
                PropertyChanges {
                    target: idleFrame
                    opacity: 0
                    visible: false
                }
                PropertyChanges {
                    target: scanFrame
                    opacity: 0
                    visible: false
                }
                PropertyChanges {
                    target: messageFrame
                    opacity: 0
                    visible: false
                }
                PropertyChanges {
                    target: printingFrame
                    opacity: 0
                    visible: false
                }
                PropertyChanges {
                    target: prepFrame
                    opacity: 0
                    visible: false
                }
                PropertyChanges {
                    target: loadingFrame
                    opacity: 1
                    visible: true
                }
            },
            State {
                name: "printingState"
                when: appstate == Main.AppState.Printing
                PropertyChanges {
                    target: idleFrame
                    opacity: 0
                    visible: false
                }
                PropertyChanges {
                    target: scanFrame
                    opacity: 0
                    visible: false
                }
                PropertyChanges {
                    target: messageFrame
                    opacity: 0
                    visible: false
                }
                PropertyChanges {
                    target: printingFrame
                    opacity: 1
                    visible: true
                }
                PropertyChanges {
                    target: prepFrame
                    opacity: 0
                    visible: false
                }
                PropertyChanges {
                    target: loadingFrame
                    opacity: 0
                    visible: false
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
                width: 630
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
                    onClicked: {
                        backend.helpButtonClicked();
                        messageText.text = "Start by slicing your .obj file\nor selecting a sliced file\nThen scan your ID and follow the\nInstructions on screen to start your print";
                        messageFrame.nextState = Main.AppState.Idle
                        rootWindow.appstate = Main.AppState.Message
                    }
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

                RoundButton {
                    id: uploadButton
                    anchors.bottom: parent.bottom
                    anchors.horizontalCenter: parent.horizontalCenter
                    onClicked: gcodeFileDialog.open()
                    width: 200
                    height: 50
                    radius: 5
                    background: Rectangle {
                        color: parent.down ? "#6b1616" : "#871C1C"
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
                            id: uploadImage
                            source: "../resources/upload_file.svg"
                            height: 24
                            width: 24
                            fillMode: Image.PreserveAspectFit
                            anchors.verticalCenter: parent.verticalCenter
                            anchors.left: parent.left
                            anchors.leftMargin: 45
                        }

                        Text {
                            text: "Upload GCode"
                            color: "#fff"
                            anchors.left: uploadImage.right
                            anchors.leftMargin: 5
                            anchors.verticalCenter: parent.verticalCenter
                        }
                    }
                    FileDialog {
                        id: gcodeFileDialog
                        title: "Select a file"
                        nameFilters: ["GCode File (*.gcode *.bgcode *.gcode.3mf)"]
                        onAccepted: {
                            rootWindow.appstate = Main.AppState.Loading
                            backend.fileUploaded(gcodeFileDialog.selectedFile)
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
                text: (rootWindow.appstate == Main.AppState.UserScan) ? "Tap your ID or UCard" : "Tap Staff Card"
                font.pointSize: 40
                anchors.bottom: parent.bottom
                anchors.bottomMargin: 20
                anchors.right: parent.right
                anchors.rightMargin: 50
                color: "#ffffff"
            }

            RoundButton {
                id: cancelScanButton
                anchors.bottom: parent.bottom
                anchors.left: parent.left
                anchors.leftMargin: 10
                anchors.bottomMargin: 10
                onClicked: {
                    rootWindow.appstate = Main.AppState.Idle
                    printInfoText.text = "No print information found"
                }
                width: 160
                height: 40
                radius: 5
                background: Rectangle {
                    color: parent.down ? "#6b1616" : "#871C1C"
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

                Text {
                    text: "Cancel"
                    color: "#fff"
                    anchors.centerIn: parent
                }

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
            property int nextState: Main.AppState.Idle

            Text {
                id: messageText
                text: "Unknown Error"
                font.pointSize: 24
                anchors.horizontalCenter: parent.horizontalCenter;
                anchors.verticalCenter: parent.verticalCenter;
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                color: "#fff"
            }

            Connections {
                target: backend
                function onMessageReq(msg, btnText, newState) {
                    messageText.text = msg
                    messageAcceptText.text = btnText
                    messageFrame.nextState = newState
                }
            }

            RoundButton {
                id: acceptMessageButton
                anchors.bottom: parent.bottom
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.bottomMargin: 50
                onClicked: {
                    rootWindow.appstate = messageFrame.nextState
                    messageText.text = "Unknown Error"
                    messageFrame.nextState = Main.AppState.Idle
                }
                width: 160
                height: 40
                radius: 5
                background: Rectangle {
                    color: parent.down ? "#6b1616" : "#871C1C"
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

                Text {
                    id: messageAcceptText
                    text: "OK"
                    color: "#fff"
                    anchors.centerIn: parent
                }

            }

            RoundButton {
                id: cancelMessageButton
                visible: parent.nextState != Main.AppState.Idle
                anchors.bottom: parent.bottom
                anchors.left: parent.left
                anchors.leftMargin: 10
                anchors.bottomMargin: 10
                onClicked: {
                    rootWindow.appstate = Main.AppState.Idle
                    message.text = "Unknown Error"
                }
                width: 160
                height: 40
                radius: 5
                background: Rectangle {
                    color: parent.down ? "#6b1616" : "#871C1C"
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

                Text {
                    text: "Cancel"
                    color: "#fff"
                    anchors.centerIn: parent
                }

            }
        }

        Item {
            id: prepFrame
            anchors.fill: parent
            visible: false
            opacity: 0

            Text {
                id: prepLabel
                text: "Print Information"
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                anchors.horizontalCenter: parent.horizontalCenter;
                anchors.top: parent.top
                anchors.topMargin: 160
                font.pointSize: 36
                font.bold: true
                color: "#fff"
            }

            Rectangle {
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.top: prepLabel.bottom
                anchors.topMargin: 20
                width: printInfoText.implicitWidth + 20
                height: printInfoText.implicitHeight + 20
                color : "#871C1C"
                border.width: 2
                border.color: "#fff"
                radius: 2
                Text {
                    id: printInfoText
                    text: "No print information found"
                    horizontalAlignment: Text.AlignLeft
                    verticalAlignment: Text.AlignVCenter
                    anchors.horizontalCenter: parent.horizontalCenter
                    anchors.top: parent.top
                    anchors.topMargin: 10
                    color: "#fff"
                    font.pointSize: 18
                }
            }

            Connections {
                target: backend
                function onPrintInfoLoaded(printInfo) {
                    console.log("Print has been loaded!")
                    console.log(printInfo)
                    let op = `Filename: ${printInfo.filename}\nPrinter: ${printInfo.printer}\nFilament: ${printInfo.filamentType}\nWeight: ${printInfo.weight}g\nDuration: ${printInfo.duration}`;
                    if (printInfo.hasOwnProperty("printSettings")) op += `\nPrint Settings: ${printInfo.printSettings}`
                    printInfoText.text = op
                }
            }

            Connections {
                target: octoprintemulator
                function onJobInfoLoaded(printInfo) {
                    console.log("Job has been loaded!")
                    console.log(printInfo)
                    let op = `Filename: ${printInfo.filename}\nPrinter: ${printInfo.printer}\nFilament: ${printInfo.filamentType}\nWeight: ${printInfo.weight}g\nDuration: ${printInfo.duration}`;
                    if (printInfo.hasOwnProperty("printSettings")) op += `\nPrint Settings: ${printInfo.printSettings}`
                    printInfoText.text = op
                }
            }

            RoundButton {
                id: cancelPrepButton
                anchors.bottom: parent.bottom
                anchors.left: parent.left
                anchors.leftMargin: 10
                anchors.bottomMargin: 10
                onClicked: {
                    rootWindow.appstate = Main.AppState.Idle
                    printInfoText.text = "No print information found"
                }
                width: 160
                height: 40
                radius: 5
                background: Rectangle {
                    color: parent.down ? "#6b1616" : "#871C1C"
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

                Text {
                    text: "Cancel"
                    color: "#fff"
                    anchors.centerIn: parent
                }

            }

            RoundButton {
                id: beginPrintButton
                anchors.bottom: parent.bottom
                anchors.right: parent.right
                anchors.rightMargin: 10
                anchors.bottomMargin: 10
                onClicked: {
                    rootWindow.appstate = Main.AppState.UserScan
                    printInfoText.text = "No print information found"
                }
                width: 160
                height: 40
                radius: 5
                background: Rectangle {
                    color: parent.down ? "#6b1616" : "#871C1C"
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

                Text {
                    text: "Print"
                    color: "#fff"
                    anchors.centerIn: parent
                }

            }
        }

        Item {
            id: loadingFrame
            anchors.fill: parent
            visible: false
            opacity: 0

            Image {
                id: spinnyThing
                source: "../resources/progress_activity.svg"
                width: 96
                height: 96
                anchors.centerIn: parent
                fillMode: Image.PreserveAspectFit

                NumberAnimation on rotation {
                    loops: Animation.Infinite
                    from: 0
                    to: 360
                    duration: 250
                }
            }

            Text {
                text: "Loading"
                font.pointSize: 24
                anchors.top: spinnyThing.bottom
                anchors.topMargin: 15
                color: "#fff"
                anchors.horizontalCenter: parent.horizontalCenter
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
