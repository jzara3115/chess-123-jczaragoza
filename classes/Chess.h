#pragma once

#include "Game.h"
#include "Grid.h"
#include "Bitboard.h"
#include <vector>

constexpr int pieceSize = 80;

enum ChessPiece
{
    NoPiece,
    Pawn,
    Knight,
    Bishop,
    Rook,
    Queen,
    King
};

class Chess : public Game
{
public:
    Chess();
    ~Chess();

    void setUpBoard() override;

    bool canBitMoveFrom(Bit &bit, BitHolder &src) override;
    bool canBitMoveFromTo(Bit &bit, BitHolder &src, BitHolder &dst) override;
    bool actionForEmptyHolder(BitHolder &holder) override;

    void stopGame() override;

    Player *checkForWinner() override;
    bool checkForDraw() override;

    std::string initialStateString() override;
    std::string stateString() override;
    void setStateString(const std::string &s) override;

    Grid* getGrid() override { return _grid; }

    // Move generation
    int generateMoves(BitMove* moveList, int maxMoves);
    void generatePawnMoves(BitMove* moveList, int& moveCount, int maxMoves);
    void generateKnightMoves(BitMove* moveList, int& moveCount, int maxMoves);
    void generateKingMoves(BitMove* moveList, int& moveCount, int maxMoves);
    void testMoveGeneration();

private:
    Bit* PieceForPlayer(const int playerNumber, ChessPiece piece);
    Player* ownerAt(int x, int y) const;
    void FENtoBoard(const std::string& fen);
    char pieceNotation(int x, int y) const;
    
    // Helper functions for moving
    void buildBitboards(uint64_t bitboards[2][7]);
    bool isSquareAttacked(int square, int byPlayer, uint64_t bitboards[2][7]);
    ChessPiece getPieceTypeAt(int x, int y) const;

    Grid* _grid;
};
