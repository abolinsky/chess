#include "raylib.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define BOARD_SIZE 8
#define SQUARE_SIZE 80
#define SPRITE_SIZE 16
#define HIGHLIGHT_THICKNESS 7
#define PROMOTION 2
#define BUTTON_WIDTH 120
#define BUTTON_HEIGHT 40

enum PieceColor { white, black };
enum PieceType { pawn, knight, rook, bishop, queen, king, none };
enum MoveValidity { invalid, valid, capture, promotion };

typedef struct {
    Rectangle bounds;
    const char* text;
    Color color;
} Button;

Button retryButton = {{250, 240, BUTTON_WIDTH, BUTTON_HEIGHT}, "Retry", DARKGRAY};
Button quitButton = {{250, 290, BUTTON_WIDTH, BUTTON_HEIGHT}, "Quit", DARKGRAY};

typedef struct {
    enum PieceType type;
    enum PieceColor color;
} ChessPiece;

ChessPiece board[BOARD_SIZE][BOARD_SIZE];
enum PieceColor turn = white;
Vector2 selectedSquare = { -1, -1 };
bool isGameOver = false;

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

void SwitchTurn() {
    turn = turn == white ? black : white;
}

Rectangle getPieceSourceRect(enum PieceType type) {
    return (Rectangle){ (int)type * SPRITE_SIZE, 0, SPRITE_SIZE, SPRITE_SIZE };
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

    return piece;
}

void PopulateBoard() {
    for (int y = 0; y < BOARD_SIZE; ++y) {
        for (int x = 0; x < BOARD_SIZE; ++x) {
            board[y][x] = CreatePiece(startingLocations[y][x]);
        }
    }
}

void HighlightSelectedSquare() {
    if (selectedSquare.x != -1 && selectedSquare.y != -1) {
        DrawRectangleLinesEx((Rectangle){selectedSquare.x * SQUARE_SIZE, selectedSquare.y * SQUARE_SIZE, SQUARE_SIZE, SQUARE_SIZE}, HIGHLIGHT_THICKNESS, GOLD);
    }
}

void DrawBoard() {
    for (int y = 0; y < BOARD_SIZE; ++y) {
        for (int x = 0; x < BOARD_SIZE; ++x) {
            Color squareColor = ((x + y) % 2 == 0) ? LIGHTGRAY : DARKGREEN;
            DrawRectangle(x * SQUARE_SIZE, y * SQUARE_SIZE, SQUARE_SIZE, SQUARE_SIZE, squareColor);
        }
    }

    HighlightSelectedSquare();
}

void DrawPiece(Texture2D pieces, ChessPiece piece, int x, int y) {
    Rectangle destRect = { x * SQUARE_SIZE, y * SQUARE_SIZE, SQUARE_SIZE, SQUARE_SIZE};
    Vector2 origin = { 0, 0 };
    DrawTexturePro(pieces, getPieceSourceRect(piece.type), destRect, origin, 0.0f, WHITE);
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

enum MoveValidity IsValidPawnMove(Vector2 start, Vector2 end) {
    int direction = (board[(int)start.y][(int)start.x].color == white) ? -1 : 1;
    
    if ((board[(int)start.y][(int)start.x].color == white && end.y == 0) ||
        (board[(int)start.y][(int)start.x].color == black && end.y == 7)) {
        return promotion;
    }

    // TODO: Implement "en passant"

    if (end.y - start.y == direction && end.x == start.x && board[(int)end.y][(int)end.x].type == none) {
        return valid;
    }
    
    if (fabsf(end.y - start.y) == 2 && end.x == start.x && board[(int)end.y][(int)end.x].type == none 
        && (start.y == 6 || start.y == 1)) {
        return valid;
    }
    
    if (end.y - start.y == direction && fabsf(end.x - start.x) == 1 
        && board[(int)end.y][(int)end.x].type != none && IsOppositeColor(board[(int)start.y][(int)start.x], board[(int)end.y][(int)end.x])) {
        return capture;
    }
    
    return invalid;
}

enum MoveValidity IsValidRookMove(Vector2 start, Vector2 end) {
    if (start.x != end.x && start.y != end.y) {
        return invalid;
    }

    int stepX = (end.x > start.x) ? 1 : (end.x < start.x ? -1 : 0);
    int stepY = (end.y > start.y) ? 1 : (end.y < start.y ? -1 : 0);

    int x, y;
    for (x = start.x + stepX, y = start.y + stepY; x != end.x || y != end.y; x += stepX, y += stepY) {
        if (board[(int)y][(int)x].type != none) {
            return invalid;
        }
    }

    if (board[(int)end.y][(int)end.x].type != none) {
        if (IsOppositeColor(board[(int)start.y][(int)start.x], board[(int)end.y][(int)end.x])) {
            return capture;
        } else {
            return invalid;
        }
    } else {
        return valid;
    }
}

enum MoveValidity IsValidKnightMove(Vector2 start, Vector2 end) {
    int dx = fabsf(end.x - start.x);
    int dy = fabsf(end.y - start.y);

    if ((dx == 2 && dy == 1) || (dx == 1 && dy == 2)) {
        if (board[(int)end.y][(int)end.x].type == none) {
            return valid;
        } else if (IsOppositeColor(board[(int)start.y][(int)start.x], board[(int)end.y][(int)end.x])) {
            return capture;
        }
    }
    return invalid;
}

enum MoveValidity IsValidBishopMove(Vector2 start, Vector2 end) {
    int dx = end.x - start.x;
    int dy = end.y - start.y;

    if (abs(dx) != abs(dy)) {
        return invalid;
    }

    int stepX = (dx > 0) ? 1 : -1;
    int stepY = (dy > 0) ? 1 : -1;

    int x, y;
    for (x = start.x + stepX, y = start.y + stepY; x != end.x; x += stepX, y += stepY) {
        if (board[(int)y][(int)x].type != none) {
            return invalid;
        }
    }

    if (board[(int)end.y][(int)end.x].type != none) {
        if (IsOppositeColor(board[(int)start.y][(int)start.x], board[(int)end.y][(int)end.x])) {
            return capture;
        } else {
            return invalid;
        }
    } else {
        return valid;
    }
}

enum MoveValidity IsValidQueenMove(Vector2 start, Vector2 end) {
    return IsValidRookMove(start, end) || IsValidBishopMove(start, end);
}

enum MoveValidity IsValidKingMove(Vector2 start, Vector2 end) {
    int dx = fabsf(end.x - start.x);
    int dy = fabsf(end.y - start.y);

    if ((dx == 1 || dx == 0) && (dy == 1 || dy == 0)) {
        if (board[(int)end.y][(int)end.x].type == none) {
            return valid;
        } else if (IsOppositeColor(board[(int)start.y][(int)start.x], board[(int)end.y][(int)end.x])) {
            return capture;
        }
    }
    return invalid;

    // TODO: Implement castling
}

enum MoveValidity IsValidMove(ChessPiece* piece, Vector2 start, Vector2 end) {
    if (piece->type == rook) { return IsValidRookMove(start, end); }
    else if (piece->type == pawn) { return IsValidPawnMove(start, end); }
    else if (piece->type == knight) { return IsValidKnightMove(start, end); }
    else if (piece->type == bishop) { return IsValidBishopMove(start, end); }
    else if (piece->type == queen) { return IsValidQueenMove(start, end); }
    else if (piece->type == king) { return IsValidKingMove(start, end); }
    return invalid;
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
        ChessPiece* clickedPiece = &board[(int)clickedSquare.y][(int)clickedSquare.x];
        if (clickedPiece->type != none && turn == clickedPiece->color) {
            selectedSquare = clickedSquare;
        } else if (selectedSquare.x != -1 && selectedSquare.y != -1) {
            if (selectedSquare.x != clickedSquare.x || selectedSquare.y != clickedSquare.y) {
                ChessPiece* selectedPiece = &board[(int)selectedSquare.y][(int)selectedSquare.x];
                enum MoveValidity result = IsValidMove(selectedPiece, selectedSquare, clickedSquare);
                if (result) {
                    if (clickedPiece->type == king) {
                        isGameOver = true;
                        return;
                    }

                    *clickedPiece = *selectedPiece;
                    *selectedPiece = (ChessPiece){.type = none};

                    if (result == promotion) {
                        // TODO: Present a choice menu
                        clickedPiece->type = queen;
                    }

                    if (result == capture) {
                        // TODO: Something on capture
                    }

                    SwitchTurn();
                }
            }
            selectedSquare = (Vector2){ -1, -1 };
        }
    }
}

bool LoadBoard(const char *filename) {
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

void HandleGameEnd() {
    if (isGameOver) {
        ClearBackground(RAYWHITE);

        DrawText("GAME OVER", 260, 180, 40, GOLD);

        DrawRectangleRec(retryButton.bounds, retryButton.color);
        DrawText(retryButton.text, retryButton.bounds.x + 20, retryButton.bounds.y + 10, 20, BLACK);

        DrawRectangleRec(quitButton.bounds, quitButton.color);
        DrawText(quitButton.text, quitButton.bounds.x + 20, quitButton.bounds.y + 10, 20, BLACK);
        
        if (CheckCollisionPointRec(GetMousePosition(), retryButton.bounds) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            isGameOver = false;
            PopulateBoard();
        } else if (CheckCollisionPointRec(GetMousePosition(), quitButton.bounds) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            CloseWindow();
            exit(0);
        }
    }
}

int main(int argc, char** argv) {
    int screenWidth = BOARD_SIZE * SQUARE_SIZE;
    int screenHeight = BOARD_SIZE * SQUARE_SIZE;

    InitWindow(screenWidth, screenHeight, "Chess");

    Texture2D chessPiecesBlack = LoadTexture("sprites/BlackPieces_Wood.png");
    Texture2D chessPiecesWhite = LoadTexture("sprites/WhitePieces_Wood.png");

    SetTargetFPS(60);

    if (argc == 2) {
        LoadBoard(argv[1]);
    }
    PopulateBoard();

    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(RAYWHITE);
        DrawBoard();
        DrawPieces(chessPiecesWhite, chessPiecesBlack);
        HandlePieces();
        HandleGameEnd();
        EndDrawing();
    }

    UnloadTexture(chessPiecesWhite);
    UnloadTexture(chessPiecesBlack);
    CloseWindow();

    return 0;
}