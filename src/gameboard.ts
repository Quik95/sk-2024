import spriteUrl from "/chess_set.svg?url"
import {ChessPiece, ChessPieceColor, ChessPieceType} from "./ChessPiece.ts";
import {
    gameEndedHandler,
    InBoundMessageType,
    OutBoundMessageType,
    sendServerRequest,
    updateTurnIndicator
} from "./main.ts";


class ChessSet {
    private readonly img: HTMLImageElement
    private readonly squareSize: number

    constructor(squareSize: number) {
        const image = new Image()
        image.src = spriteUrl
        this.img = image
        this.squareSize = squareSize
    }

    public drawPiece(ctx: CanvasRenderingContext2D, offsetX: number, offsetY: number, pieceType: ChessPieceType, color: ChessPieceColor) {
        if (pieceType == ChessPieceType.EMPTY)
            return

        const pieceSize = 45
        const imgOffsetY = color == ChessPieceColor.WHITE ? 0 : pieceSize
        let imgOffsetX;
        switch (pieceType) {
            case ChessPieceType.PAWN:
                imgOffsetX = pieceSize * 5
                break
            case ChessPieceType.ROOK:
                imgOffsetX = pieceSize * 4
                break
            case ChessPieceType.KNIGHT:
                imgOffsetX = pieceSize * 3
                break;
            case ChessPieceType.BISHOP:
                imgOffsetX = pieceSize * 2
                break;
            case ChessPieceType.QUEEN:
                imgOffsetX = pieceSize * 1
                break;
            case ChessPieceType.KING:
                imgOffsetX = pieceSize * 0
                break;
            default:
                throw new Error("invalid piece type")
        }

        ctx.drawImage(this.img, imgOffsetX, imgOffsetY, pieceSize, pieceSize, offsetX * this.squareSize, offsetY * this.squareSize, this.squareSize, this.squareSize)
    }
}

type PiecePosition = [number, number]

enum State {
    NONE,
    PIECE_SELECTED
}

class BoardState {
    public readonly state: (ChessPiece | null)[]

    constructor() {
        this.state = []
        for (let i = 0; i < 8 * 8; i++) {
            this.state.push(null)
        }
    }

    private static xyIdx(x: number, y: number): number {
        if (x < 0 || x > 7 || y < 0 || y > 7) {
            throw new Error(`out of bounds access: x: ${x} y: ${y}`)
        }
        return y * 8 + x
    }

    public set(p: ChessPiece, x: number, y: number) {
        this.state[BoardState.xyIdx(x, y)] = p
    }

    public get(x: number, y: number): ChessPiece | null {
        return this.state[BoardState.xyIdx(x, y)]
    }

    public delete(x: number, y: number) {
        this.state[BoardState.xyIdx(x, y)] = null
    }

    public initializeGameboard() {
        [ChessPieceType.ROOK, ChessPieceType.KNIGHT, ChessPieceType.BISHOP, ChessPieceType.QUEEN, ChessPieceType.KING, ChessPieceType.BISHOP, ChessPieceType.KNIGHT, ChessPieceType.ROOK]
            .forEach(
                (pieceType, i) => {
                    this.set(ChessPiece.FromTypeAndColor(pieceType, ChessPieceColor.BLACK), i, 0)
                }
            )

        for (let i = 0; i < 8; i++) {
            this.set(ChessPiece.FromTypeAndColor(ChessPieceType.PAWN, ChessPieceColor.BLACK), i, 1)
        }

        [ChessPieceType.ROOK, ChessPieceType.KNIGHT, ChessPieceType.BISHOP, ChessPieceType.QUEEN, ChessPieceType.KING, ChessPieceType.BISHOP, ChessPieceType.KNIGHT, ChessPieceType.ROOK]
            .forEach((pieceType, i) =>
                this.set(ChessPiece.FromTypeAndColor(pieceType, ChessPieceColor.WHITE), i, 7)
            )
        for (let i = 0; i < 8; i++) {
            this.set(ChessPiece.FromTypeAndColor(ChessPieceType.PAWN, ChessPieceColor.WHITE), i, 6)
        }
    }
}

export default class Gameboard {
    private ctx: CanvasRenderingContext2D
    private chessSet: ChessSet
    private squareSize: number
    private boardState: BoardState
    private interactionState: State = State.NONE
    private selectedPiece: [number, number] | null = null

    private readonly chessBoardDarkBackground = "#b96d40"
    private readonly chessBoardLightBackground = "#ffdda1"
    private readonly chessBoardHoverColor = "#985277"

    constructor() {
        const canvas = document.getElementById("gameBoard") as HTMLCanvasElement
        this.ctx = canvas.getContext("2d") as CanvasRenderingContext2D
        this.squareSize = canvas.width / 8
        this.chessSet = new ChessSet(this.squareSize)
        this.boardState = new BoardState()
        this.boardState.initializeGameboard()

        canvas.addEventListener("mousedown", this.handleMouseClick)
    }

    public draw() {
        this.drawChessboardPattern()

        this.boardState.state
            .forEach((p, index) => {
                if (p == null)
                    return;

                let canvasX = index % 8
                let canvasY = Math.floor(index / 8)

                const piece = p as ChessPiece
                this.chessSet.drawPiece(this.ctx, canvasX, canvasY, piece.Type, piece.Color)
                this.ctx.filter = "none"
            })
    }

    private drawChessboardPattern() {
        this.ctx.fillStyle = "white"
        this.ctx.fillRect(0, 0, 600, 600)

        const d = this.squareSize
        for (let y = 0; y < 8; y += 1) {
            for (let x = 0; x < 8; x += 1) {
                this.ctx.fillStyle = (x + y) % 2 == 0 ? this.chessBoardLightBackground : this.chessBoardDarkBackground

                if (this.selectedPiece != null && this.selectedPiece[0] == x && this.selectedPiece[1] == y) {
                    this.ctx.fillStyle = this.chessBoardHoverColor
                }
                this.ctx.fillRect(x * d, y * d, d, d)
            }
        }
    }

    private handleMouseClick = (e: MouseEvent) => {
        const canvas = e.target as HTMLCanvasElement
        const bx = canvas?.getBoundingClientRect();
        const position = {
            x: (e.clientX) - bx.left,
            y: (e.clientY) - bx.top,
        };

        const boardTileX = Math.floor(position.x / this.squareSize)
        const boardTileY = Math.floor(position.y / this.squareSize)
        if (this.interactionState == State.PIECE_SELECTED) {
            if (this.selectedPiece == undefined)
                throw new Error("something went horribly wrong")

            if (!(this.selectedPiece[0] == boardTileX && this.selectedPiece[1] == boardTileY)) {
                this.movePiece(this.selectedPiece, [boardTileX, boardTileY])
            }
            this.interactionState = State.NONE
            this.selectedPiece = null
        } else {
            this.interactionState = State.PIECE_SELECTED
            this.selectedPiece = [boardTileX, boardTileY]
        }
        this.draw()
    }

    public synchronizeBoardState(newBoardState: number[][]) {
        const boardState = new BoardState()
        for (let y = 0; y < 8; y += 1) {
            for (let x = 0; x < 8; x += 1) {
                const pieceType = newBoardState[y][x]
                const piece: ChessPiece = ChessPiece.FromValue(pieceType);
                boardState.set(piece, x, y)
            }
        }
        this.boardState = boardState
        this.draw()
    }

    private async movePiece(pos: PiecePosition, newPos: PiecePosition) {
        const [x, y] = pos
        const [newX, newY] = newPos
        const piece = this.boardState.get(x, y)
        if (piece == undefined) {
            throw new Error("this should not have happened")
        }

        this.boardState.delete(x, y)
        this.boardState.set(piece, newX, newY)

        const res = await sendServerRequest<MovePieceResponse>(OutBoundMessageType.MOVE_PIECE, {
            move: {
                from: [x, y],
                to: [newX, newY]
            }
        })
        if (res.messageType == InBoundMessageType.GAME_STATE_SYNC) {
            this.synchronizeBoardState(res.board)
            updateTurnIndicator(res.currentTurn)
            console.log("invalid move, synchronizing board state...")
            return
        }
        updateTurnIndicator(piece.Color == ChessPieceColor.WHITE ? ChessPieceColor.BLACK : ChessPieceColor.WHITE)

        if (res.messageType == InBoundMessageType.GAME_ENDED) {
            gameEndedHandler(res.winner)
        }
    }

    public disableInteractivity() {
        const canvas = document.getElementById("gameBoard") as HTMLCanvasElement
        canvas.removeEventListener("mousedown", this.handleMouseClick)
    }
}

type MovePieceResponse =
    { messageType: InBoundMessageType.MOVE_ACCEPTED }
    | {
    messageType: InBoundMessageType.GAME_STATE_SYNC,
    board: number[][],
    currentTurn: ChessPieceColor
} | {
    messageType: InBoundMessageType.GAME_ENDED,
    winner: ChessPieceColor,
    board: number[][]
}