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
    bool gameHasAI() override;
    void updateAI() override;

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
    std::vector<BitMove> generateAllMoves();
    std::vector<BitMove> generateAllMovesForPlayer(int playerNumber);
    int generateMoves(BitMove* moveList, int maxMoves);
    void generatePawnMoves(std::vector<BitMove>& moves, int currentPlayer, uint64_t bitboards[2][7]);
    void generateKnightMoves(std::vector<BitMove>& moves, int currentPlayer, uint64_t bitboards[2][7]);
    void generateKingMoves(std::vector<BitMove>& moves, int currentPlayer, uint64_t bitboards[2][7]);
    void generateRookMoves(std::vector<BitMove>& moves, int currentPlayer, uint64_t bitboards[2][7]);
    void generateBishopMoves(std::vector<BitMove>& moves, int currentPlayer, uint64_t bitboards[2][7]);
    void generateQueenMoves(std::vector<BitMove>& moves, int currentPlayer, uint64_t bitboards[2][7]);
    void testMoveGeneration();

    // Configure whether AI plays White (0) or Black (1). Use -1 for human vs human.
    void setAIPlayerChoice(int playerNumber);

private:
    Bit* PieceForPlayer(const int playerNumber, ChessPiece piece);
    Player* ownerAt(int x, int y) const;
    void FENtoBoard(const std::string& fen);
    char pieceNotation(int x, int y) const;
    
    // Helper functions for moving
    void buildBitboards(uint64_t bitboards[2][7]);
    ChessPiece getPieceTypeAt(int x, int y) const;
    void addSlidingMoves(std::vector<BitMove>& moves, ChessPiece pieceType, int currentPlayer, uint64_t pieces, uint64_t friendlyPieces, uint64_t enemyPieces);
    int evaluateBoard() const;
    int evaluateState(const std::string& state) const;
    int evaluateForPlayer(int playerNumber) const;
    int evaluateStateForPlayer(const std::string& state, int playerNumber) const;
    int pieceSquareScore(ChessPiece piece, int square, int playerNumber) const;
    int negamax(const std::string& state, int depth, int alpha, int beta, int playerNumber);
    bool applyMoveOnBoard(const BitMove& move);
    std::string applyMoveToState(const std::string& state, const BitMove& move) const;
    std::vector<BitMove> generateAllMovesForPlayerFromState(const std::string& state, int currentPlayer) const;
    BitMove findBestMove(int depth, int playerNumber);

    Grid* _grid;
    int _aiPlayerChoice;
    int _searchDepth;
};
