export enum ChessPieceType {
    EMPTY = 0,
    PAWN = 1,
    KNIGHT = 2,
    BISHOP = 3,
    ROOK=4 ,
    QUEEN = 5,
    KING = 6
}

export enum ChessPieceColor {
    BLACK = 1, WHITE = 0
}

export class ChessPiece {
    private readonly value: number;

    public static FromValue(value: number): ChessPiece {
        return new ChessPiece(value)
    }

    public static FromTypeAndColor(type: ChessPieceType, color: ChessPieceColor): ChessPiece {
        return new ChessPiece(type | color << 3)
    }

    private constructor(value: number) {
        this.value = value;
    }

    public get Value(): number {
        return this.value
    }

    public get Type(): ChessPieceType {
        return this.value & 0b111
    }

    public get Color(): ChessPieceColor {
        return this.value >> 3 & 0b1
    }
}
