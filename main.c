#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>

#define SCREEN_WIDTH 640
#define SCREEN_HEIGHT 640
#define TILE_SIZE 80 // Each square in the 8x8 chessboard

// Define pieces with numbers
#define EMPTY 0
#define PAWN 1
#define KNIGHT 2
#define BISHOP 3
#define ROOK 4
#define QUEEN 5
#define KING 6

#define WHITE 10 // Add 10 to distinguish white pieces
#define BLACK 20 // Add 20 to distinguish black pieces

// Evaluation values
#define PAWN_VALUE 1
#define KNIGHT_VALUE 3
#define BISHOP_VALUE 3
#define ROOK_VALUE 5
#define QUEEN_VALUE 9
#define KING_VALUE 100

int board[8][8] = {
    {BLACK+ROOK, BLACK+KNIGHT, BLACK+BISHOP, BLACK+QUEEN, BLACK+KING, BLACK+BISHOP, BLACK+KNIGHT, BLACK+ROOK},
    {BLACK+PAWN, BLACK+PAWN, BLACK+PAWN, BLACK+PAWN, BLACK+PAWN, BLACK+PAWN, BLACK+PAWN, BLACK+PAWN},
    {EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY},
    {EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY},
    {EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY},
    {EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY},
    {WHITE+PAWN, WHITE+PAWN, WHITE+PAWN, WHITE+PAWN, WHITE+PAWN, WHITE+PAWN, WHITE+PAWN, WHITE+PAWN},
    {WHITE+ROOK, WHITE+KNIGHT, WHITE+BISHOP, WHITE+QUEEN, WHITE+KING, WHITE+BISHOP, WHITE+KNIGHT, WHITE+ROOK}
};

// Declare necessary functions
void move_piece(int startRow, int startCol, int endRow, int endCol) {
    board[endRow][endCol] = board[startRow][startCol];
    board[startRow][startCol] = EMPTY;  // Clear the starting position
}
void save_board_to_history();
void undo_move();
void add_move_to_history(int startRow, int startCol, int endRow, int endCol, int capturedPiece);
void display_move_history();
void reset_game();

// Stores history of moves
typedef struct {
    int startRow, startCol, endRow, endCol;
    int capturedPiece;
    int moveNumber;
    char moveNotation[10];
} Move;

Move moveHistory[500]; // Large enough to store up to 500 moves
int moveCount = 0;

// Undo functionality - backup board state
int boardHistory[500][8][8];
int currentMoveIndex = 0;

int highlightedMoves[8][8] = {0}; // Stores valid moves for the selected piece

SDL_Texture* whitePawnTexture = NULL;
SDL_Texture* blackPawnTexture = NULL;
SDL_Texture* whiteRookTexture = NULL;
SDL_Texture* blackRookTexture = NULL;
SDL_Texture* whiteKnightTexture = NULL;
SDL_Texture* blackKnightTexture = NULL;
SDL_Texture* whiteBishopTexture = NULL;
SDL_Texture* blackBishopTexture = NULL;
SDL_Texture* whiteQueenTexture = NULL;
SDL_Texture* blackQueenTexture = NULL;
SDL_Texture* whiteKingTexture = NULL;
SDL_Texture* blackKingTexture = NULL;

SDL_Window* window = NULL;
SDL_Renderer* renderer = NULL;

int selectedRow = -1, selectedCol = -1;
int currentTurn = WHITE;  // Alternates between WHITE and BLACK

// Save the current board state to history
void save_board_to_history() {
    for (int row = 0; row < 8; row++) {
        for (int col = 0; col < 8; col++) {
            boardHistory[currentMoveIndex][row][col] = board[row][col];
        }
    }
    currentMoveIndex++;
}

// Undo the last move by restoring the previous board state
void undo_move() {
    if (currentMoveIndex > 1) {
        currentMoveIndex--;
        for (int row = 0; row < 8; row++) {
            for (int col = 0; col < 8; col++) {
                board[row][col] = boardHistory[currentMoveIndex - 1][row][col];
            }
        }
        // Remove the last move from the history
        moveCount--;
    }
}

// Add move to the history
void add_move_to_history(int startRow, int startCol, int endRow, int endCol, int capturedPiece) {
    moveHistory[moveCount].startRow = startRow;
    moveHistory[moveCount].startCol = startCol;
    moveHistory[moveCount].endRow = endRow;
    moveHistory[moveCount].endCol = endCol;
    moveHistory[moveCount].capturedPiece = capturedPiece;
    moveHistory[moveCount].moveNumber = moveCount + 1;

    // Generate a simple algebraic notation for the move (e.g., "e2 to e4")
    char columns[] = "abcdefgh";
    sprintf(moveHistory[moveCount].moveNotation, "%c%d to %c%d",
            columns[startCol], 8 - startRow, columns[endCol], 8 - endRow);

    moveCount++;
}

// Display the move history in the console
void display_move_history() {
    printf("\nMove History:\n");
    for (int i = 0; i < moveCount; i++) {
        printf("%d. %s\n", moveHistory[i].moveNumber, moveHistory[i].moveNotation);
    }
}

// Reset the game to its starting position
void reset_game() {
    int initialBoard[8][8] = {
        {BLACK+ROOK, BLACK+KNIGHT, BLACK+BISHOP, BLACK+QUEEN, BLACK+KING, BLACK+BISHOP, BLACK+KNIGHT, BLACK+ROOK},
        {BLACK+PAWN, BLACK+PAWN, BLACK+PAWN, BLACK+PAWN, BLACK+PAWN, BLACK+PAWN, BLACK+PAWN, BLACK+PAWN},
        {EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY},
        {EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY},
        {EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY},
        {EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY},
        {WHITE+PAWN, WHITE+PAWN, WHITE+PAWN, WHITE+PAWN, WHITE+PAWN, WHITE+PAWN, WHITE+PAWN, WHITE+PAWN},
        {WHITE+ROOK, WHITE+KNIGHT, WHITE+BISHOP, WHITE+QUEEN, WHITE+KING, WHITE+BISHOP, WHITE+KNIGHT, WHITE+ROOK}
    };
    memcpy(board, initialBoard, sizeof(initialBoard));
    moveCount = 0;
    currentMoveIndex = 0;
    save_board_to_history(); // Save the initial state to history
}

// SDL-related functions for rendering
SDL_Texture* loadTexture(const char* path) {
    SDL_Texture* newTexture = NULL;
    SDL_Surface* loadedSurface = SDL_LoadBMP(path);
    if (loadedSurface == NULL) {
        printf("Unable to load image %s! SDL Error: %s\n", path, SDL_GetError());
    } else {
        newTexture = SDL_CreateTextureFromSurface(renderer, loadedSurface);
        SDL_FreeSurface(loadedSurface);
    }
    return newTexture;
}

// Highlight the selected piece
void highlight_tile(int row, int col) {
    SDL_Rect tile = {col * TILE_SIZE, row * TILE_SIZE, TILE_SIZE, TILE_SIZE};
    SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);  // Green highlight
    SDL_RenderDrawRect(renderer, &tile);
}

void draw_chessboard() {
    for (int row = 0; row < 8; row++) {
        for (int col = 0; col < 8; col++) {
            SDL_Rect tile = {col * TILE_SIZE, row * TILE_SIZE, TILE_SIZE, TILE_SIZE};
            if ((row + col) % 2 == 0) {
                SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255); // White tile
            } else {
                SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255); // Black tile
            }
            SDL_RenderFillRect(renderer, &tile);
        }
    }
}

void draw_piece(SDL_Texture* texture, int row, int col) {
    SDL_Rect destRect = {col * TILE_SIZE, row * TILE_SIZE, TILE_SIZE, TILE_SIZE};
    SDL_RenderCopy(renderer, texture, NULL, &destRect);
}

void render_pieces() {
    for (int row = 0; row < 8; row++) {
        for (int col = 0; col < 8; col++) {
            int piece = board[row][col];
            if (piece != EMPTY) {
                if (piece == WHITE + PAWN) draw_piece(whitePawnTexture, row, col);
                if (piece == BLACK + PAWN) draw_piece(blackPawnTexture, row, col);
                if (piece == WHITE + ROOK) draw_piece(whiteRookTexture, row, col);
                if (piece == BLACK + ROOK) draw_piece(blackRookTexture, row, col);
                if (piece == WHITE + KNIGHT) draw_piece(whiteKnightTexture, row, col);
                if (piece == BLACK + KNIGHT) draw_piece(blackKnightTexture, row, col);
                if (piece == WHITE + BISHOP) draw_piece(whiteBishopTexture, row, col);
                if (piece == BLACK + BISHOP) draw_piece(blackBishopTexture, row, col);
                if (piece == WHITE + QUEEN) draw_piece(whiteQueenTexture, row, col);
                if (piece == BLACK + QUEEN) draw_piece(blackQueenTexture, row, col);
                if (piece == WHITE + KING) draw_piece(whiteKingTexture, row, col);
                if (piece == BLACK + KING) draw_piece(blackKingTexture, row, col);
            }
        }
    }
}

int init_SDL() {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
        return 0;
    }
    window = SDL_CreateWindow("Chess Game", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
    if (window == NULL) {
        printf("Window could not be created! SDL_Error: %s\n", SDL_GetError());
        return 0;
    }
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (renderer == NULL) {
        printf("Renderer could not be created! SDL Error: %s\n", SDL_GetError());
        return 0;
    }

    whitePawnTexture = loadTexture("C:/Users/Salim Sadman/Documents/C/chess/images/white_pawn.bmp");
    blackPawnTexture = loadTexture("C:/Users/Salim Sadman/Documents/C/chess/images/black_pawn.bmp");
    whiteRookTexture = loadTexture("C:/Users/Salim Sadman/Documents/C/chess/images/white_rook.bmp");
    blackRookTexture = loadTexture("C:/Users/Salim Sadman/Documents/C/chess/images/black_rook.bmp");
    whiteKnightTexture = loadTexture("C:/Users/Salim Sadman/Documents/C/chess/images/white_knight.bmp");
    blackKnightTexture = loadTexture("C:/Users/Salim Sadman/Documents/C/chess/images/black_knight.bmp");
    whiteBishopTexture = loadTexture("C:/Users/Salim Sadman/Documents/C/chess/images/white_bishop.bmp");
    blackBishopTexture = loadTexture("C:/Users/Salim Sadman/Documents/C/chess/images/black_bishop.bmp");
    whiteQueenTexture = loadTexture("C:/Users/Salim Sadman/Documents/C/chess/images/white_queen.bmp");
    blackQueenTexture = loadTexture("C:/Users/Salim Sadman/Documents/C/chess/images/black_queen.bmp");
    whiteKingTexture = loadTexture("C:/Users/Salim Sadman/Documents/C/chess/images/white_king.bmp");
    blackKingTexture = loadTexture("C:/Users/Salim Sadman/Documents/C/chess/images/black_king.bmp");

    if (!whitePawnTexture || !blackPawnTexture || !whiteRookTexture || !blackRookTexture ||
        !whiteKnightTexture || !blackKnightTexture || !whiteBishopTexture || !blackBishopTexture ||
        !whiteQueenTexture || !blackQueenTexture || !whiteKingTexture || !blackKingTexture) {
        printf("Failed to load some piece textures!\n");
        return 0;
    }
    return 1;
}

void close_SDL() {
    SDL_DestroyTexture(whitePawnTexture);
    SDL_DestroyTexture(blackPawnTexture);
    SDL_DestroyTexture(whiteRookTexture);
    SDL_DestroyTexture(blackRookTexture);
    SDL_DestroyTexture(whiteKnightTexture);
    SDL_DestroyTexture(blackKnightTexture);
    SDL_DestroyTexture(whiteBishopTexture);
    SDL_DestroyTexture(blackBishopTexture);
    SDL_DestroyTexture(whiteQueenTexture);
    SDL_DestroyTexture(blackQueenTexture);
    SDL_DestroyTexture(whiteKingTexture);
    SDL_DestroyTexture(blackKingTexture);

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

int main(int argc, char* args[]) {
    if (!init_SDL()) {
        printf("Failed to initialize!\n");
        return 0;
    }

    reset_game();  // Set up initial board and save the initial state

    int quit = 0;
    SDL_Event e;

    while (!quit) {
        printf("Event loop running...\n"); // Debugging event loop
        while (SDL_PollEvent(&e) != 0) {
            if (e.type == SDL_QUIT) {
                printf("Quit event detected!\n"); // Debugging quit event
                quit = 1;
            } else if (e.type == SDL_MOUSEBUTTONDOWN) {
                int x, y;
                SDL_GetMouseState(&x, &y);
                int row = y / TILE_SIZE;
                int col = x / TILE_SIZE;

                if (selectedRow == -1) {
                    if (board[row][col] != EMPTY && (board[row][col] / 10) == (currentTurn / 10)) {
                        selectedRow = row;
                        selectedCol = col;
                    }
                } else {
                    if (board[row][col] == EMPTY || (board[row][col] / 10) != (currentTurn / 10)) {
                        int capturedPiece = board[row][col];  // Capture the piece if present
                        move_piece(selectedRow, selectedCol, row, col);
                        add_move_to_history(selectedRow, selectedCol, row, col, capturedPiece);
                        save_board_to_history();  // Save current board state after each move
                        display_move_history();   // Show the updated move history
                        selectedRow = -1;  // Reset selection
                        currentTurn = (currentTurn == WHITE) ? BLACK : WHITE;  // Switch turn
                    }
                }
            } else if (e.type == SDL_KEYDOWN) {
                if (e.key.keysym.sym == SDLK_u) {
                    undo_move();  // Press 'U' to undo
                } else if (e.key.keysym.sym == SDLK_r) {
                    reset_game();  // Press 'R' to reset the game
                }
            }
        }

        // Clear screen
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        // Draw chessboard and pieces
        draw_chessboard();
        render_pieces();

        // Highlight selected piece
        if (selectedRow != -1 && selectedCol != -1) {
            highlight_tile(selectedRow, selectedCol);
        }

        // Update screen
        SDL_RenderPresent(renderer);
    }

    close_SDL();
    return 0;
}
