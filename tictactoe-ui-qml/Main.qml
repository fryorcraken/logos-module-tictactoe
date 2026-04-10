import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: root
    color: "#1a1a1a"

    // Game state
    property var board: [0, 0, 0, 0, 0, 0, 0, 0, 0]  // 0=empty, 1=X, 2=O
    property int gameStatus: 0   // 0=ongoing, 1=X wins, 2=O wins, 3=draw
    property int currentPlayer: 1  // 1=X, 2=O

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 24
        spacing: 16

        // ── Title ──────────────────────────────────────────────
        Text {
            text: "Tic-Tac-Toe"
            font.pixelSize: 20
            font.weight: Font.DemiBold
            color: "#e0e0e0"
            Layout.alignment: Qt.AlignHCenter
        }

        // ── Status ─────────────────────────────────────────────
        Text {
            id: statusLabel
            text: statusText()
            font.pixelSize: 14
            color: "#e0e0e0"
            Layout.alignment: Qt.AlignHCenter
        }

        // ── Board grid ─────────────────────────────────────────
        GridLayout {
            columns: 3
            rowSpacing: 6
            columnSpacing: 6
            Layout.alignment: Qt.AlignHCenter

            Repeater {
                model: 9

                Button {
                    id: cellBtn
                    Layout.preferredWidth: 96
                    Layout.preferredHeight: 96

                    property int cellValue: root.board[index]

                    text: cellValue === 1 ? "X" : (cellValue === 2 ? "O" : "")
                    font.pixelSize: 32
                    font.weight: Font.Bold
                    enabled: cellValue === 0 && root.gameStatus === 0

                    onClicked: {
                        var row = Math.floor(index / 3)
                        var col = index % 3
                        callPlay(row, col)
                    }

                    background: Rectangle {
                        color: cellBtn.hovered && cellBtn.enabled ? "#3a3a3a" : "#2a2a2a"
                        border.color: cellBtn.enabled ? "#555" : "#333"
                        border.width: 1
                        radius: 4
                    }

                    contentItem: Text {
                        text: cellBtn.text
                        font: cellBtn.font
                        color: cellBtn.cellValue === 1 ? "#4a9eff"
                             : cellBtn.cellValue === 2 ? "#ff6b6b"
                             : (cellBtn.enabled ? "#e0e0e0" : "#666")
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                }
            }
        }

        // ── New Game button ────────────────────────────────────
        Button {
            id: newGameBtn
            text: "New Game"
            font.pixelSize: 12
            Layout.fillWidth: true

            onClicked: callNewGame()

            background: Rectangle {
                color: newGameBtn.hovered ? "#3a3a3a" : "#2a2a2a"
                border.color: "#555"
                border.width: 1
                radius: 4
            }

            contentItem: Text {
                text: newGameBtn.text
                font: newGameBtn.font
                color: "#e0e0e0"
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }
        }

        Item { Layout.fillHeight: true }
    }

    // ── Logos bridge helpers ───────────────────────────────────

    function callModule(method, args) {
        if (typeof logos === "undefined" || !logos.callModule) {
            return -1
        }
        return logos.callModule("tictactoe", method, args)
    }

    function callNewGame() {
        callModule("newGame", [])
        refreshBoard()
    }

    function callPlay(row, col) {
        callModule("play", [row, col])
        refreshBoard()
    }

    function refreshBoard() {
        var newBoard = []
        for (var r = 0; r < 3; r++) {
            for (var c = 0; c < 3; c++) {
                var cell = callModule("getCell", [r, c])
                newBoard.push(cell)
            }
        }
        root.board = newBoard
        root.gameStatus = callModule("status", [])
        root.currentPlayer = callModule("currentPlayer", [])
    }

    function statusText() {
        if (root.gameStatus === 1) return "X wins!"
        if (root.gameStatus === 2) return "O wins!"
        if (root.gameStatus === 3) return "Draw!"
        return root.currentPlayer === 1 ? "X's turn" : "O's turn"
    }

    Component.onCompleted: callNewGame()
}
