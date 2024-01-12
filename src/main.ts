import './style.css'
import Gameboard from "./gameboard.ts";
import {ChessPieceColor} from "./ChessPiece.ts";

const CANVAS_INNER_HTML = `
      <h1 id="colorTurn" class="white-turn">White Turn</h1>
      <h5>You are playing: <span id="playerColor"></span></h5>
      <canvas id="gameBoard" width="600px" height="600px"></canvas>
      <button id="changeGame">Connect to a new game</button>
`

const FORM_INNER_HTML = `
      <h1>Connect to a game</h1>
      <form>
        <label for="gameId">Game ID:</label>
        <input type="text" id="gameId">
        <button type="submit">Join Game</button>
      </form>
`

export enum InBoundMessageType {
    WAIT_FOR_OTHER_PLAYER = 0,
    GAME_STARTED,
    GAME_STATE_SYNC,
    MOVE_ACCEPTED,
    GAME_ENDED,
    PLAYER_DISCONNECTED
}

export enum OutBoundMessageType {
    JOIN_GAME = 128,
    GAME_STATE_SYNC,
    MOVE_PIECE,
    DISCONNECT,
}

export type ServerResponse = {
    messageType: InBoundMessageType
    gameId: string
    playerId: string
}

let gameStatePollIntervalId: number | null = null

let gameID: string | null = null
let playerID: string | null = null
let gameboard: Gameboard | null = null

export const URL = "http://localhost:2137"
export const FETCH_BASE_OPTS = {method: "POST"}

function gameJoinHandler(e: SubmitEvent) {
    e.preventDefault()
    const form = (e.target as HTMLElement).querySelector("#gameId") as HTMLFormElement
    if (form == null)
        throw new Error("something went very wrong")

    const gameId = form.value
    form.removeEventListener("submit", gameJoinHandler)
    connectToGame(gameId)
}

function showGameSelectPage() {
    gameStatePollIntervalId && clearInterval(gameStatePollIntervalId)

    const changeGameButton = document.getElementById("changeGame")
    if (changeGameButton != null)
        changeGameButton.removeEventListener("click", showGameSelectPage)
    
    document.getElementById("app")!.innerHTML = FORM_INNER_HTML
    const gameJoinForm = document.querySelector("form")
    gameJoinForm!.addEventListener("submit", gameJoinHandler)
}

type GameJoinResponse = {
    playerColor: ChessPieceColor
}

async function connectToGame(gameId: string) {
    console.log(`Game ID: ${gameId}`)
    try {
        const resp = await sendServerRequest<GameJoinResponse>(OutBoundMessageType.JOIN_GAME, {gameId: gameId})
        gameID = resp.gameId
        playerID = resp.playerId
        gameStatePollIntervalId = setInterval(pollForGameState, 1000)
        if (resp.messageType == InBoundMessageType.WAIT_FOR_OTHER_PLAYER) {
            document.getElementById("app")!.appendChild(document.createTextNode("Waiting for other player..."))
            return
        }

        showGameBoard(resp.playerColor)

    } catch (e) {
        console.error(e)
        return
    }
}

function showGameBoard(color: ChessPieceColor) {
    document.getElementById("app")!.innerHTML = CANVAS_INNER_HTML
    document.getElementById("changeGame")!.addEventListener("click", showGameSelectPage)

    const el = document.getElementById("playerColor") as HTMLElement
    el.innerText = color == ChessPieceColor.WHITE ? "White" : "Black"
    el.classList.add(color == ChessPieceColor.WHITE ? "white-turn" : "black-turn")

    gameboard = new Gameboard()
    gameboard.draw()
}

type GameStateSyncResponse = {
    messageType: InBoundMessageType.WAIT_FOR_OTHER_PLAYER
}  | {
    messageType: InBoundMessageType.GAME_STATE_SYNC | InBoundMessageType.GAME_STARTED,
    board: number[][],
    currentTurn: ChessPieceColor,
    playerColor: ChessPieceColor
} | {
    messageType: InBoundMessageType.GAME_ENDED,
    winner: ChessPieceColor,
    board: number[][]
} | {
    messageType: InBoundMessageType.PLAYER_DISCONNECTED,
}

export function sendServerRequest<T>(msgType: OutBoundMessageType, data?: object): Promise<ServerResponse & T> {
    const opts = {
        ...FETCH_BASE_OPTS,
        body: JSON.stringify(
    {
                messageType: msgType,
                gameId: gameID,
                playerId: playerID,
                ...data || {}
            }
        )
    }

    return fetch(URL, opts).then(r => r.json())
}

async function pollForGameState() {
    try {
        const resp = await sendServerRequest<GameStateSyncResponse>(OutBoundMessageType.GAME_STATE_SYNC);
        if (resp.messageType === InBoundMessageType.WAIT_FOR_OTHER_PLAYER)
            return

        if (resp.messageType == InBoundMessageType.GAME_ENDED) {
            gameboard?.synchronizeBoardState(resp.board)
            gameEndedHandler(resp.winner)
            return
        }

        if (resp.messageType == InBoundMessageType.PLAYER_DISCONNECTED) {
            opponentDisconnectedHandler()
            return
        }

        if (gameboard === null) {
            showGameBoard(resp.playerColor)
        }

        gameboard?.synchronizeBoardState(resp.board)
        updateTurnIndicator(resp.currentTurn)
    } catch(e) {
        console.error(e)
    }
}


export function updateTurnIndicator(color: ChessPieceColor) {
    const colorTurnHTML = document.getElementById("colorTurn") as HTMLElement
    colorTurnHTML.classList.remove("black-turn")
    colorTurnHTML.classList.remove("white-turn")
    colorTurnHTML.classList.add(color == ChessPieceColor.WHITE ? "white-turn" : "black-turn")
    colorTurnHTML.innerText = color == ChessPieceColor.WHITE ? "White Turn" : "Black Turn"
}

export function gameEndedHandler(winner: ChessPieceColor) {
    gameStatePollIntervalId && clearInterval(gameStatePollIntervalId)
    gameboard?.disableInteractivity()
    gameboard = null

    const colorTurnHTML = document.getElementById("colorTurn") as HTMLElement
    colorTurnHTML.classList.remove("black-turn")
    colorTurnHTML.classList.remove("white-turn")
    colorTurnHTML.classList.add(winner == ChessPieceColor.WHITE ? "white-turn" : "black-turn")
    colorTurnHTML.innerText = winner == ChessPieceColor.WHITE
    ? "White has won!!!"
        : "Black has won!!!"

    sendServerRequest<{}>(OutBoundMessageType.DISCONNECT)
}

function opponentDisconnectedHandler() {
    gameStatePollIntervalId && clearInterval(gameStatePollIntervalId)
    gameboard?.disableInteractivity()
    gameboard = null

    const colorTurnHTML = document.getElementById("colorTurn") as HTMLElement
    colorTurnHTML.classList.remove("black-turn")
    colorTurnHTML.classList.remove("white-turn")
    colorTurnHTML.classList.add("black-turn")
    colorTurnHTML.innerText = "Opponent disconnected"

    sendServerRequest<{}>(OutBoundMessageType.DISCONNECT)
}

showGameSelectPage()