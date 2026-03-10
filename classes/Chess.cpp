#include "Chess.h"
#include <limits>
#include <cmath>
#include <iostream>
#include <algorithm>
#include <cctype>

Chess::Chess()
{
    _grid = new Grid(8, 8);
}

Chess::~Chess()
{
    delete _grid;
}

char Chess::pieceNotation(int x, int y) const
{
    const char *wpieces = { "0PNBRQK" };
    const char *bpieces = { "0pnbrqk" };
    Bit *bit = _grid->getSquare(x, y)->bit();
    char notation = '0';
    if (bit) {
        notation = bit->gameTag() < 128 ? wpieces[bit->gameTag()] : bpieces[bit->gameTag()-128];
    }
    return notation;
}

Bit* Chess::PieceForPlayer(const int playerNumber, ChessPiece piece)
{
    const char* pieces[] = { "pawn.png", "knight.png", "bishop.png", "rook.png", "queen.png", "king.png" };

    Bit* bit = new Bit();
    const char* pieceName = pieces[piece - 1];
    std::string spritePath = std::string("") + (playerNumber == 0 ? "w_" : "b_") + pieceName;
    bit->LoadTextureFromFile(spritePath.c_str());
    bit->setOwner(getPlayerAt(playerNumber));
    bit->setSize(pieceSize, pieceSize);

    return bit;
}

void Chess::setUpBoard()
{
    setNumberOfPlayers(2);
    _gameOptions.rowX = 8;
    _gameOptions.rowY = 8;

    _grid->initializeChessSquares(pieceSize, "boardsquare.png");
    FENtoBoard("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR");

    startGame();
}

void Chess::testMoveGeneration()
{
    BitMove moveList[256];
    int moveCount = generateMoves(moveList, 256);
    
    std::cout << "Generated " << moveCount << " moves for " 
              << (getCurrentPlayer()->playerNumber() == 0 ? "White" : "Black") << "\n\n";
    
    // Print first 20 moves
    int movesToPrint = moveCount < 20 ? moveCount : 20;
    for (int i = 0; i < movesToPrint; i++) {
        int fromFile = moveList[i].from % 8;
        int fromRank = moveList[i].from / 8;
        int toFile = moveList[i].to % 8;
        int toRank = moveList[i].to / 8;
        
        const char* pieceNames[] = {"", "Pawn", "Knight", "Bishop", "Rook", "Queen", "King"};
        
        std::cout << "Move " << (i + 1) << ": " << pieceNames[moveList[i].piece] 
                  << " from " << (char)('a' + fromFile) << (fromRank + 1)
                  << " to " << (char)('a' + toFile) << (toRank + 1) << "\n";
    }
    
    if (moveCount >= 20) {
        std::cout << "\n moveList array containing first 20 moves\n";
    }
}


void Chess::FENtoBoard(const std::string& fen) {
    
    std::string boardPosition = fen;
    size_t spacePos = fen.find(' ');
    if (spacePos != std::string::npos) {
        boardPosition = fen.substr(0, spacePos);
    }
    
    _grid->forEachSquare([](ChessSquare* square, int x, int y) {
        square->destroyBit();
    });
    
    // Within each rank, goes from file a (x=0) to file h (x=7)
    int x = 0;
    int y = 7;  //top rank
    
    for (char c : boardPosition) {
        if (c == '/') {
            y--;
            x = 0;
        } else if (c >= '1' && c <= '8') {
            x += (c - '0');
        } else {
            ChessPiece pieceType = NoPiece;
            int playerNumber = -1;
            
            // Find piece type
            char upperC = toupper(c);
            switch (upperC) {
                case 'P': pieceType = Pawn; break;
                case 'N': pieceType = Knight; break;
                case 'B': pieceType = Bishop; break;
                case 'R': pieceType = Rook; break;
                case 'Q': pieceType = Queen; break;
                case 'K': pieceType = King; break;
            }
            
            // uppercase = white = player 0, lowercase = black = player 1
            playerNumber = (c >= 'A' && c <= 'Z') ? 0 : 1;
            
            // Create and place the piece
            if (pieceType != NoPiece && x < 8 && y >= 0) {
                Bit* piece = PieceForPlayer(playerNumber, pieceType);
                piece->setGameTag(pieceType + (playerNumber * 128));
                ChessSquare* square = _grid->getSquare(x, y);
                square->setBit(piece);
                piece->setParent(square);
                piece->moveTo(square->getPosition());
            }
            
            x++;
        }
    }
}

bool Chess::actionForEmptyHolder(BitHolder &holder)
{
    return false;
}

bool Chess::canBitMoveFrom(Bit &bit, BitHolder &src)
{
    // need to implement friendly/unfriendly in bit so for now this hack
    int currentPlayer = getCurrentPlayer()->playerNumber() * 128;
    int pieceColor = bit.gameTag() & 128;
    if (pieceColor == currentPlayer) return true;
    return false;
}

bool Chess::canBitMoveFromTo(Bit &bit, BitHolder &src, BitHolder &dst)
{
    ChessSquare* srcSquare = dynamic_cast<ChessSquare*>(&src);
    ChessSquare* dstSquare = dynamic_cast<ChessSquare*>(&dst);
    
    if (!srcSquare || !dstSquare) {
        return false;
    }
    
    int fromSquare = srcSquare->getSquareIndex();
    int toSquare = dstSquare->getSquareIndex();

    ChessPiece pieceType = (ChessPiece)(bit.gameTag() & 0x7F);
    std::vector<BitMove> legalMoves = generateAllMoves();
    
    for (const BitMove& move : legalMoves) {
        if (move.from == fromSquare &&
            move.to == toSquare &&
            move.piece == pieceType) {
            return true;
        }
    }
    
    return false;
}

ChessPiece Chess::getPieceTypeAt(int x, int y) const
{
    if (x < 0 || x >= 8 || y < 0 || y >= 8) {
        return NoPiece;
    }
    
    auto square = _grid->getSquare(x, y);
    if (!square || !square->bit()) {
        return NoPiece;
    }

    return (ChessPiece)(square->bit()->gameTag() & 0x7F);
}

void Chess::buildBitboards(uint64_t bitboards[2][7])
{
    for (int player = 0; player < 2; player++) {
        for (int piece = 0; piece < 7; piece++) {
            bitboards[player][piece] = 0ULL;
        }
    }
    
    //Scan the board and populate it
    _grid->forEachSquare([&](ChessSquare* square, int x, int y) {
        Bit* bit = square->bit();
        if (bit) {
            int squareIndex = y * 8 + x;
            int playerNumber = (bit->gameTag() & 128) ? 1 : 0;
            ChessPiece pieceType = (ChessPiece)(bit->gameTag() & 0x7F);
            
            if (pieceType >= Pawn && pieceType <= King) {
                bitboards[playerNumber][pieceType] |= (1ULL << squareIndex);
            }
        }
    });
}

std::vector<BitMove> Chess::generateAllMoves()
{
    int currentPlayer = getCurrentPlayer()->playerNumber();
    uint64_t bitboards[2][7];
    buildBitboards(bitboards);

    std::vector<BitMove> moves;
    moves.reserve(256);

    generatePawnMoves(moves, currentPlayer, bitboards);
    generateKnightMoves(moves, currentPlayer, bitboards);
    generateBishopMoves(moves, currentPlayer, bitboards);
    generateRookMoves(moves, currentPlayer, bitboards);
    generateQueenMoves(moves, currentPlayer, bitboards);
    generateKingMoves(moves, currentPlayer, bitboards);

    return moves;
}

int Chess::generateMoves(BitMove* moveList, int maxMoves)
{
    std::vector<BitMove> moves = generateAllMoves();
    int moveCount = std::min((int)moves.size(), maxMoves);
    for (int i = 0; i < moveCount; i++) {
        moveList[i] = moves[i];
    }
    return moveCount;
}

void Chess::generatePawnMoves(std::vector<BitMove>& moves, int currentPlayer, uint64_t bitboards[2][7])
{
    uint64_t pawns = bitboards[currentPlayer][Pawn];
    uint64_t allPieces = 0ULL;
    uint64_t enemyPieces = 0ULL;

    for (int piece = Pawn; piece <= King; piece++) {
        allPieces |= bitboards[0][piece];
        allPieces |= bitboards[1][piece];
        enemyPieces |= bitboards[1 - currentPlayer][piece];
    }

    uint64_t emptySquares = ~allPieces;

    BitboardElement pawnBB(pawns);
    pawnBB.forEachBit([&](int square) {
        int rank = square / 8;
        int file = square % 8;

        if (currentPlayer == 0) {
            int targetSquare = square + 8;
            if (targetSquare < 64 && (emptySquares & (1ULL << targetSquare))) {
                moves.push_back(BitMove(square, targetSquare, Pawn));
                if (rank == 1) {
                    int doublePushSquare = square + 16;
                    if (emptySquares & (1ULL << doublePushSquare)) {
                        moves.push_back(BitMove(square, doublePushSquare, Pawn));
                    }
                }
            }

            if (file > 0) {
                int captureSquare = square + 7;
                if (captureSquare < 64 && (enemyPieces & (1ULL << captureSquare))) {
                    moves.push_back(BitMove(square, captureSquare, Pawn));
                }
            }
            if (file < 7) {
                int captureSquare = square + 9;
                if (captureSquare < 64 && (enemyPieces & (1ULL << captureSquare))) {
                    moves.push_back(BitMove(square, captureSquare, Pawn));
                }
            }
        }
        else {
            int targetSquare = square - 8;
            if (targetSquare >= 0 && (emptySquares & (1ULL << targetSquare))) {
                moves.push_back(BitMove(square, targetSquare, Pawn));
                if (rank == 6) {
                    int doublePushSquare = square - 16;
                    if (emptySquares & (1ULL << doublePushSquare)) {
                        moves.push_back(BitMove(square, doublePushSquare, Pawn));
                    }
                }
            }

            if (file > 0) {
                int captureSquare = square - 9;
                if (captureSquare >= 0 && (enemyPieces & (1ULL << captureSquare))) {
                    moves.push_back(BitMove(square, captureSquare, Pawn));
                }
            }
            if (file < 7) {
                int captureSquare = square - 7;
                if (captureSquare >= 0 && (enemyPieces & (1ULL << captureSquare))) {
                    moves.push_back(BitMove(square, captureSquare, Pawn));
                }
            }
        }
    });
}

void Chess::generateKnightMoves(std::vector<BitMove>& moves, int currentPlayer, uint64_t bitboards[2][7])
{
    uint64_t knights = bitboards[currentPlayer][Knight];
    uint64_t friendlyPieces = 0ULL;
    for (int piece = Pawn; piece <= King; piece++) {
        friendlyPieces |= bitboards[currentPlayer][piece];
    }

    BitboardElement knightBB(knights);
    knightBB.forEachBit([&](int square) {
        uint64_t attacks = KnightAttacks[square] & ~friendlyPieces;
        BitboardElement attackBB(attacks);
        attackBB.forEachBit([&](int targetSquare) {
            moves.push_back(BitMove(square, targetSquare, Knight));
        });
    });
}

void Chess::generateKingMoves(std::vector<BitMove>& moves, int currentPlayer, uint64_t bitboards[2][7])
{
    uint64_t kings = bitboards[currentPlayer][King];
    uint64_t friendlyPieces = 0ULL;
    for (int piece = Pawn; piece <= King; piece++) {
        friendlyPieces |= bitboards[currentPlayer][piece];
    }

    BitboardElement kingBB(kings);
    kingBB.forEachBit([&](int square) {
        uint64_t attacks = KingAttacks[square] & ~friendlyPieces;
        BitboardElement attackBB(attacks);
        attackBB.forEachBit([&](int targetSquare) {
            moves.push_back(BitMove(square, targetSquare, King));
        });
    });
}

void Chess::addSlidingMoves(std::vector<BitMove>& moves, ChessPiece pieceType, int currentPlayer, uint64_t pieces, uint64_t friendlyPieces, uint64_t enemyPieces)
{
    const int rookDirections[4][2] = { {1,0}, {-1,0}, {0,1}, {0,-1} };
    const int bishopDirections[4][2] = { {1,1}, {1,-1}, {-1,1}, {-1,-1} };

    const int (*directions)[2] = nullptr;
    int directionCount = 0;

    if (pieceType == Rook) {
        directions = rookDirections;
        directionCount = 4;
    }
    else if (pieceType == Bishop) {
        directions = bishopDirections;
        directionCount = 4;
    }
    else {
        static const int queenDirections[8][2] = {
            {1,0}, {-1,0}, {0,1}, {0,-1}, {1,1}, {1,-1}, {-1,1}, {-1,-1}
        };
        directions = queenDirections;
        directionCount = 8;
    }

    BitboardElement pieceBB(pieces);
    pieceBB.forEachBit([&](int fromSquare) {
        int fromX = fromSquare % 8;
        int fromY = fromSquare / 8;

        for (int d = 0; d < directionCount; d++) {
            int x = fromX + directions[d][0];
            int y = fromY + directions[d][1];

            while (x >= 0 && x < 8 && y >= 0 && y < 8) {
                int toSquare = y * 8 + x;
                uint64_t targetMask = 1ULL << toSquare;

                if (friendlyPieces & targetMask) {
                    break;
                }

                moves.push_back(BitMove(fromSquare, toSquare, pieceType));

                if (enemyPieces & targetMask) {
                    break;
                }

                x += directions[d][0];
                y += directions[d][1];
            }
        }
    });
}

void Chess::generateRookMoves(std::vector<BitMove>& moves, int currentPlayer, uint64_t bitboards[2][7])
{
    uint64_t rooks = bitboards[currentPlayer][Rook];
    uint64_t friendlyPieces = 0ULL;
    uint64_t enemyPieces = 0ULL;
    for (int piece = Pawn; piece <= King; piece++) {
        friendlyPieces |= bitboards[currentPlayer][piece];
        enemyPieces |= bitboards[1 - currentPlayer][piece];
    }
    addSlidingMoves(moves, Rook, currentPlayer, rooks, friendlyPieces, enemyPieces);
}

void Chess::generateBishopMoves(std::vector<BitMove>& moves, int currentPlayer, uint64_t bitboards[2][7])
{
    uint64_t bishops = bitboards[currentPlayer][Bishop];
    uint64_t friendlyPieces = 0ULL;
    uint64_t enemyPieces = 0ULL;
    for (int piece = Pawn; piece <= King; piece++) {
        friendlyPieces |= bitboards[currentPlayer][piece];
        enemyPieces |= bitboards[1 - currentPlayer][piece];
    }
    addSlidingMoves(moves, Bishop, currentPlayer, bishops, friendlyPieces, enemyPieces);
}

void Chess::generateQueenMoves(std::vector<BitMove>& moves, int currentPlayer, uint64_t bitboards[2][7])
{
    uint64_t queens = bitboards[currentPlayer][Queen];
    uint64_t friendlyPieces = 0ULL;
    uint64_t enemyPieces = 0ULL;
    for (int piece = Pawn; piece <= King; piece++) {
        friendlyPieces |= bitboards[currentPlayer][piece];
        enemyPieces |= bitboards[1 - currentPlayer][piece];
    }
    addSlidingMoves(moves, Queen, currentPlayer, queens, friendlyPieces, enemyPieces);
}

void Chess::stopGame()
{
    _grid->forEachSquare([](ChessSquare* square, int x, int y) {
        square->destroyBit();
    });
}

Player* Chess::ownerAt(int x, int y) const
{
    if (x < 0 || x >= 8 || y < 0 || y >= 8) {
        return nullptr;
    }

    auto square = _grid->getSquare(x, y);
    if (!square || !square->bit()) {
        return nullptr;
    }
    return square->bit()->getOwner();
}

Player* Chess::checkForWinner()
{
    return nullptr;
}

bool Chess::checkForDraw()
{
    return false;
}

std::string Chess::initialStateString()
{
    return stateString();
}

std::string Chess::stateString()
{
    std::string s;
    s.reserve(64);
    _grid->forEachSquare([&](ChessSquare* square, int x, int y) {
            s += pieceNotation( x, y );
        }
    );
    return s;}

void Chess::setStateString(const std::string &s)
{
    _grid->forEachSquare([&](ChessSquare* square, int x, int y) {
        int index = y * 8 + x;
        char playerNumber = s[index] - '0';
        if (playerNumber) {
            square->setBit(PieceForPlayer(playerNumber - 1, Pawn));
        } else {
            square->setBit(nullptr);
        }
    });
}
