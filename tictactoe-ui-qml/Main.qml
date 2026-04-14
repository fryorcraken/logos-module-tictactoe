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
    property var winLine: []  // indices of the 3 winning cells

    // Multiplayer state
    property bool multiplayerEnabled: false
    property bool deliveryStarted: false
    property int messagesSent: 0
    property int messagesReceived: 0
    property string contentTopic: "/tictactoe/1/moves/json"

    // Subscribe to delivery module events for multiplayer
    Connections {
        target: typeof logos !== "undefined" ? logos : null
        function onModuleEventReceived(moduleName, eventName, data) {
            if (moduleName === "delivery_module" && eventName === "messageReceived")
                handleDeliveryMessage(data)
        }
    }

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
                    property bool isWinCell: root.winLine.indexOf(index) >= 0

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
                        color: cellBtn.isWinCell ? "#1a3a1a"
                             : cellBtn.hovered && cellBtn.enabled ? "#3a3a3a" : "#2a2a2a"
                        border.color: cellBtn.isWinCell ? "#4aff4a"
                                    : cellBtn.enabled ? "#555" : "#333"
                        border.width: cellBtn.isWinCell ? 2 : 1
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

        // ── Multiplayer section ────────────────────────────────
        Button {
            id: mpToggle
            text: root.multiplayerEnabled ? "Disable Multiplayer" : "Enable Multiplayer"
            font.pixelSize: 12
            Layout.fillWidth: true

            onClicked: {
                if (root.multiplayerEnabled)
                    disableMultiplayer()
                else
                    enableMultiplayer()
            }

            background: Rectangle {
                color: mpToggle.hovered ? "#3a3a3a" : "#2a2a2a"
                border.color: "#555"
                border.width: 1
                radius: 4
            }

            contentItem: Text {
                text: mpToggle.text
                font: mpToggle.font
                color: "#e0e0e0"
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }
        }

        Text {
            text: root.multiplayerEnabled
                  ? "Delivery: running — sent: " + root.messagesSent + ", received: " + root.messagesReceived
                  : "Multiplayer: off"
            font.pixelSize: 10
            color: root.multiplayerEnabled ? "#4aff4a" : "#888"
            Layout.alignment: Qt.AlignHCenter
        }

        Item { Layout.fillHeight: true }
    }

    // ── Logos bridge helpers ───────────────────────────────────

    function callModule(method, args) {
        if (typeof logos === "undefined" || !logos.callModule)
            return -1
        return logos.callModule("tictactoe", method, args)
    }

    function callDelivery(method, args) {
        if (typeof logos === "undefined" || !logos.callModule)
            return -1
        return logos.callModule("delivery_module", method, args)
    }

    function callNewGame() {
        callModule("newGame", [])
        broadcastNewGame()
        refreshBoard()
    }

    function callPlay(row, col) {
        var player = root.currentPlayer
        var result = callModule("play", [row, col])
        if (result === 0)
            broadcastMove(row, col, player)
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
        root.winLine = (root.gameStatus === 1 || root.gameStatus === 2) ? findWinLine() : []
    }

    function findWinLine() {
        var lines = [
            [0,1,2], [3,4,5], [6,7,8],  // rows
            [0,3,6], [1,4,7], [2,5,8],  // cols
            [0,4,8], [2,4,6]            // diagonals
        ]
        for (var i = 0; i < lines.length; i++) {
            var a = root.board[lines[i][0]]
            var b = root.board[lines[i][1]]
            var c = root.board[lines[i][2]]
            if (a !== 0 && a === b && b === c)
                return lines[i]
        }
        return []
    }

    function statusText() {
        if (root.gameStatus === 1) return "X wins!"
        if (root.gameStatus === 2) return "O wins!"
        if (root.gameStatus === 3) return "Draw!"
        return root.currentPlayer === 1 ? "X's turn" : "O's turn"
    }

    // ── Multiplayer ───────────────────────────────────────────

    function enableMultiplayer() {
        if (root.multiplayerEnabled) return
        var config = '{"logLevel":"INFO","mode":"Core","preset":"logos.dev","relay":true}'
        callDelivery("createNode", [config])
        callDelivery("start", [])
        callDelivery("subscribe", [root.contentTopic])
        if (typeof logos !== "undefined" && logos.onModuleEvent)
            logos.onModuleEvent("delivery_module", "messageReceived")
        root.deliveryStarted = true
        root.multiplayerEnabled = true
    }

    function disableMultiplayer() {
        if (!root.multiplayerEnabled) return
        callDelivery("unsubscribe", [root.contentTopic])
        callDelivery("stop", [])
        root.deliveryStarted = false
        root.multiplayerEnabled = false
        root.messagesSent = 0
        root.messagesReceived = 0
    }

    function broadcastMove(row, col, player) {
        if (!root.multiplayerEnabled || !root.deliveryStarted) return
        var msg = JSON.stringify({"t": "m", "r": row, "c": col, "p": player})
        var payload = Qt.btoa(msg)
        callDelivery("send", [root.contentTopic, payload])
        root.messagesSent++
    }

    function broadcastNewGame() {
        if (!root.multiplayerEnabled || !root.deliveryStarted) return
        var msg = JSON.stringify({"t": "n"})
        var payload = Qt.btoa(msg)
        callDelivery("send", [root.contentTopic, payload])
        root.messagesSent++
    }

    function handleDeliveryMessage(data) {
        if (!root.multiplayerEnabled || data.length < 3) return
        try {
            // data[2] is the base64-encoded message payload (delivery module convention)
            var deliveryPayload = Qt.atob(data[2])
            var jsonStr = Qt.atob(deliveryPayload)
            var msg = JSON.parse(jsonStr)
            if (msg.t === "m") {
                callModule("play", [msg.r, msg.c])
                root.messagesReceived++
                refreshBoard()
            } else if (msg.t === "n") {
                callModule("newGame", [])
                root.messagesReceived++
                refreshBoard()
            }
        } catch (e) {
            // Ignore unparseable messages (e.g., protobuf from C++ UI)
        }
    }

    Component.onCompleted: callNewGame()
}
