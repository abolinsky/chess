#include "raylib.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>

#define BOARD_SIZE 8
#define SQUARE_SIZE 80
#define SPRITE_SIZE 16
#define HIGHLIGHT_THICKNESS 7

enum PieceColor { white, black };
enum PieceType { pawn, knight, rook, bishop, queen, king, none };

typedef struct {
    Rectangle sourceRect;
    enum PieceType type;
    enum PieceColor color;
} ChessPiece;

ChessPiece board[BOARD_SIZE][BOARD_SIZE];
enum PieceColor turn = white;
Vector2 selectedPiece = { -1, -1 };

void SwitchTurn() {
    turn = turn == white ? black : white;
}

ChessPiece CreatePiece(char type) {
    ChessPiece piece = (ChessPiece) { .type = none };
    piece.color = isupper(type) ? white : black;
    type = tolower(type);

    if (type == 'p') piece.type = pawn;
    else if (type == 'r') piece.type = rook;
    else if (type == 'n') piece.type = knight;
    else if (type == 'b') piece.type = bishop;
    else if (type == 'q') piece.type = queen;
    else if (type == 'k') piece.type = king;

    piece.sourceRect = (Rectangle){ (int)piece.type * SPRITE_SIZE, 0, SPRITE_SIZE, SPRITE_SIZE };
    return piece;
}

void PopulateBoard(char pieces[BOARD_SIZE][BOARD_SIZE]) {
    for (int y = 0; y < BOARD_SIZE; ++y) {
        for (int x = 0; x < BOARD_SIZE; ++x) {
            board[y][x] = CreatePiece(pieces[y][x]);
        }
    }
}

void HighlightSelectedPiece() {
    if (selectedPiece.x != -1 && selectedPiece.y != -1) {
        DrawRectangleLinesEx((Rectangle){selectedPiece.x * SQUARE_SIZE, selectedPiece.y * SQUARE_SIZE, SQUARE_SIZE, SQUARE_SIZE}, HIGHLIGHT_THICKNESS, GOLD);
    }
}

void DrawBoard() {
    for (int y = 0; y < BOARD_SIZE; ++y) {
        for (int x = 0; x < BOARD_SIZE; ++x) {
            Color squareColor = ((x + y) % 2 == 0) ? LIGHTGRAY : DARKGREEN;
            DrawRectangle(x * SQUARE_SIZE, y * SQUARE_SIZE, SQUARE_SIZE, SQUARE_SIZE, squareColor);
        }
    }

    HighlightSelectedPiece();
}

void DrawPiece(Texture2D pieces, ChessPiece piece, int x, int y) {
    Rectangle destRect = { x * SQUARE_SIZE, y * SQUARE_SIZE, SQUARE_SIZE, SQUARE_SIZE};
    Vector2 origin = { 0, 0 };
    DrawTexturePro(pieces, piece.sourceRect, destRect, origin, 0.0f, WHITE);
}

void DrawPieces(Texture2D whitePieces, Texture2D blackPieces) {
    for (int y = 0; y < BOARD_SIZE; ++y) {
        for (int x = 0; x < BOARD_SIZE; ++x) {
            ChessPiece piece = board[y][x];
            if (piece.type == none) {
                continue;
            }
            Texture2D texture = piece.color == white ? whitePieces : blackPieces;
            DrawPiece(texture, piece, x, y);
        }
    }
}

bool IsOppositeColor(ChessPiece piece1, ChessPiece piece2) {
    return (piece1.color != piece2.color);
}

bool IsValidPawnMove(int startX, int startY, int endX, int endY) {
    int direction = (board[startY][startX].color == white) ? -1 : 1;
    
    if (endY - startY == direction && endX == startX && board[endY][endX].type == none) {
        return true;
    }
    
    if (abs(endY - startY) == 2 && endX == startX && board[endY][endX].type == none 
        && (startY == 6 || startY == 1)) {
        return true;
    }
    
    if (endY - startY == direction && abs(endX - startX) == 1 
        && board[endY][endX].type != none && IsOppositeColor(board[startY][startX], board[endY][endX])) {
        return true;
    }
    
    // TODO: Implement "en passant" and promotion

    return false;
}

bool IsValidMove(ChessPiece piece, int startX, int startY, int endX, int endY) {
    return piece.type == pawn && IsValidPawnMove(startX, startY, endX, endY);
    // TODO: Validate other pieces
}

Vector2 GetBoardPosition(Vector2 mousePosition) {
    Vector2 boardPosition;
    boardPosition.x = (int)(mousePosition.x / SQUARE_SIZE);
    boardPosition.y = (int)(mousePosition.y / SQUARE_SIZE);
    return boardPosition;
}

void HandlePieces() {
    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        Vector2 clickedSquare = GetBoardPosition(GetMousePosition());

        if (selectedPiece.x != -1 && selectedPiece.y != -1) {
            if (selectedPiece.x != clickedSquare.x || selectedPiece.y != clickedSquare.y) {
                ChessPiece piece = board[(int)selectedPiece.y][(int)selectedPiece.x];
                if (IsValidMove(piece, selectedPiece.x, selectedPiece.y, clickedSquare.x, clickedSquare.y)) {
                    board[(int)clickedSquare.y][(int)clickedSquare.x] = board[(int)selectedPiece.y][(int)selectedPiece.x];
                    board[(int)selectedPiece.y][(int)selectedPiece.x] = (ChessPiece){.type = none};
                    SwitchTurn();
                }
            }
            selectedPiece = (Vector2){ -1, -1 };
        } else if (board[(int)clickedSquare.y][(int)clickedSquare.x].type != none && turn == board[(int)clickedSquare.y][(int)clickedSquare.x].color) {
            selectedPiece = clickedSquare;
        }
    }
}

bool LoadBoard(const char *filename, char startingLocations[BOARD_SIZE][BOARD_SIZE]) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        return false;
    }

    for (int i = 0; i < BOARD_SIZE; i++) {
        for (int j = 0; j < BOARD_SIZE; j++) {
            char ch = fgetc(file);
            if (ch == '\n') {
                ch = fgetc(file);
            }
            if (ch == '.') {
                startingLocations[i][j] = ' ';
            } else {
                startingLocations[i][j] = ch;
            }
        }
    }

    fclose(file);
    return true;
}

int main(int argc, char** argv) {
    int screenWidth = BOARD_SIZE * SQUARE_SIZE;
    int screenHeight = BOARD_SIZE * SQUARE_SIZE;

    InitWindow(screenWidth, screenHeight, "Chess");

    Texture2D chessPiecesBlack = LoadTexture("sprites/BlackPieces_Wood.png");
    Texture2D chessPiecesWhite = LoadTexture("sprites/WhitePieces_Wood.png");

    SetTargetFPS(60);

    char startingLocations[BOARD_SIZE][BOARD_SIZE] = {
        {'r','n','b','q','k','b','n','r'},
        {'p','p','p','p','p','p','p','p'},
        {' ',' ',' ',' ',' ',' ',' ',' '},
        {' ',' ',' ',' ',' ',' ',' ',' '},
        {' ',' ',' ',' ',' ',' ',' ',' '},
        {' ',' ',' ',' ',' ',' ',' ',' '},
        {'P','P','P','P','P','P','P','P'},
        {'R','N','B','Q','K','B','N','R'}
    };

    if (argc == 2) {
        LoadBoard(argv[1], startingLocations);
    }
    PopulateBoard(startingLocations);

    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(RAYWHITE);
        DrawBoard();
        DrawPieces(chessPiecesWhite, chessPiecesBlack);
        HandlePieces();
        EndDrawing();
    }

    UnloadTexture(chessPiecesWhite);
    UnloadTexture(chessPiecesBlack);
    CloseWindow();

    return 0;
}