import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Controls.Material
import PolyhydranPrintManager

Item {
    // anchors.fill: parent
    // anchors.centerIn: parent
    Item {
        height: childrenRect.height
        width: parent.width
        anchors.fill: parent
        anchors.centerIn: parent
        Text {
            id: prepLabel
            text: "Print Information"
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.top: parent.top
            anchors.topMargin: 120
            font.pointSize: 36
            font.bold: true
            color: Theme.text
        }

        Rectangle {
            anchors.top: prepLabel.bottom
            anchors.topMargin: 20
            width: printInfoText.implicitWidth + 20
            height: printInfoText.implicitHeight + 20
            color : Theme.background
            anchors.horizontalCenter: parent.horizontalCenter
            border.width: 2
            border.color: Theme.text
            radius: 2
            id: printInfoRect
            Text {
                id: printInfoText
                text: "No print information found"
                horizontalAlignment: Text.AlignLeft
                verticalAlignment: Text.AlignVCenter
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.top: parent.top
                anchors.topMargin: 10
                color: Theme.text
                font.pointSize: 18
            }
        }
        Item {
            width: printInfoRect.width
            height: 200
            anchors.top:printInfoRect.bottom
            anchors.topMargin: 12
            anchors.horizontalCenter: parent.horizontalCenter
            Text {
                text: "Filament Provider"
                width: parent.width
                horizontalAlignment : Text.AlignHCenter
                font.pointSize: 16
                color: Theme.text
                font.bold:true
                id: fpText
            }

            Item {
                anchors.top: fpText.bottom
                anchors.topMargin: 0
                anchors.horizontalCenter: parent.horizontalCenter
                width:childrenRect.width
                height:childrenRect.height
                RowLayout {
                    RadioButton {
                        checked:true
                        text: "Makerspace Filament"
                        id: msfButton
                        onClicked: {
                            backend.setLoadedPrintFilamentProvider(psfButton.checked)
                        }
                    }
                    RadioButton {
                        checked:false
                        text: "Personal Filament"
                        id: psfButton
                        onClicked: {
                            backend.setLoadedPrintFilamentProvider(psfButton.checked)
                        }
                    }
                }

                ButtonGroup {
                    id: filamentProvider
                    buttons: [msfButton, psfButton]
                }
            }
        }
    }

    Connections {
        target: backend
        function onPrintInfoLoaded(printInfo) {
            console.log("Print has been loaded!")
            console.log(printInfo)
            let op = `Filename: ${printInfo.filename}\nPrinter: ${printInfo.printer}\nFilament: ${(printInfo.hasOwnProperty("filament")) ? printInfo.filament : printInfo.filamentType}\nWeight: ${(printInfo.weight.trim().endsWith("g")) ? printInfo.weight : printInfo.weight + "g"}\nDuration: ${printInfo.duration}`;
            if (printInfo.hasOwnProperty("printerName")) op += `\nPrinter Name: ${printInfo.printerName}`
            if (printInfo.hasOwnProperty("printSettings")) op += `\nPrint Settings: ${printInfo.printSettings}`
            if (printInfo.hasOwnProperty("personalFilament")) {
                msfButton.checked = !printInfo.personalFilament
                psfButton.checked = printInfo.personalFilament
            } else {
                msfButton.checked = true
                psfButton.checked = false
            }

            printInfoText.text = op
        }
    }

    Connections {
        target: printermanager
        function onJobInfoLoaded(printInfo) {
            console.log("Job has been loaded!")
            console.log(printInfo)
            let op = `Filename: ${printInfo.filename}\nPrinter: ${printInfo.printer}\nFilament: ${(printInfo.hasOwnProperty("filament")) ? printInfo.filament : printInfo.filamentType}\nWeight: ${(printInfo.weight.trim().endsWith("g")) ? printInfo.weight : printInfo.weight + "g"}\nDuration: ${printInfo.duration}`;
            if (printInfo.hasOwnProperty("printerName")) op += `\nPrinter Name: ${printInfo.printerName}`
            if (printInfo.hasOwnProperty("printSettings")) op += `\nPrint Settings: ${printInfo.printSettings}`
            printInfoText.text = op
            msfButton.checked = true
            psfButton.checked = false

            rootWindow.flags |= Qt.WindowStaysOnTopHint
            rootWindow.show()
            rootWindow.raise()
            rootWindow.requestActivate()
            rootWindow.flags &= ~Qt.WindowStaysOnTopHint
        }
    }

    RoundButtonC {
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
        border_width: 0
        color: Theme.primary
        pressed_color : Theme.primaryActive
        label_text: "Cancel"
    }

    RoundButtonC {
        id: beginPrintButton
        anchors.bottom: parent.bottom
        anchors.right: parent.right
        anchors.rightMargin: 10
        anchors.bottomMargin: 10
        onClicked: {
            rootWindow.appstate = Main.AppState.UserScan
            printInfoText.text = "No print information found"
            backend.setLoadedPrintFilamentProvider(psfButton.checked)
        }
        width: 160
        height: 40
        radius: 5
        border_width: 0
        color: Theme.primary
        pressed_color : Theme.primaryActive
        label_text : "Print"
    }
}
