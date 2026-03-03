#include "Chess.h"
#include <limits>
#include <cmath>
#include <iostream>

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
        std::cout << "\nBreakpoint location: moveList array contains first 20 moves\n";
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
    int playerNumber = getCurrentPlayer()->playerNumber();
    
    //generate all legal moves for current player
    BitMove moveList[256];
    int moveCount = generateMoves(moveList, 256);
    
    //check if move is in the list of legal moves
    for (int i = 0; i < moveCount; i++) {
        if (moveList[i].from == fromSquare && 
            moveList[i].to == toSquare && 
            moveList[i].piece == pieceType) {
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

int Chess::generateMoves(BitMove* moveList, int maxMoves)
{
    int moveCount = 0;
    
    generatePawnMoves(moveList, moveCount, maxMoves);
    generateKnightMoves(moveList, moveCount, maxMoves);
    generateKingMoves(moveList, moveCount, maxMoves);
    
    return moveCount;
}

void Chess::generatePawnMoves(BitMove* moveList, int& moveCount, int maxMoves)
{
    int currentPlayer = getCurrentPlayer()->playerNumber();
    
    uint64_t bitboards[2][7];
    buildBitboards(bitboards);
    
    uint64_t pawns = bitboards[currentPlayer][Pawn];
    uint64_t allPieces = 0ULL;
    uint64_t enemyPieces = 0ULL;
    
    // Calculate taken squares
    for (int piece = Pawn; piece <= King; piece++) {
        allPieces |= bitboards[0][piece];
        allPieces |= bitboards[1][piece];
        enemyPieces |= bitboards[1 - currentPlayer][piece];
    }
    
    uint64_t emptySquares = ~allPieces;
    
    BitboardElement pawnBB(pawns);
    pawnBB.forEachBit([&](int square) {
        if (moveCount >= maxMoves) return;
        
        int rank = square / 8;
        int file = square % 8;
        
        if (currentPlayer == 0) { // White pawns
            int targetSquare = square + 8;
            if (targetSquare < 64 && (emptySquares & (1ULL << targetSquare))) {
                if (moveCount < maxMoves) {
                    moveList[moveCount++] = BitMove(square, targetSquare, Pawn);
                }
                
                if (rank == 1) {
                    int doublePushSquare = square + 16;
                    if (emptySquares & (1ULL << doublePushSquare)) {
                        if (moveCount < maxMoves) {
                            moveList[moveCount++] = BitMove(square, doublePushSquare, Pawn);
                        }
                    }
                }
            }
            
            // diagonal captures
            if (file > 0) {
                int captureSquare = square + 7;
                if (captureSquare < 64 && (enemyPieces & (1ULL << captureSquare))) {
                    if (moveCount < maxMoves) {
                        moveList[moveCount++] = BitMove(square, captureSquare, Pawn);
                    }
                }
            }
            if (file < 7) {
                int captureSquare = square + 9;
                if (captureSquare < 64 && (enemyPieces & (1ULL << captureSquare))) {
                    if (moveCount < maxMoves) {
                        moveList[moveCount++] = BitMove(square, captureSquare, Pawn);
                    }
                }
            }
        } else { // Black pawns
            int targetSquare = square - 8;
            if (targetSquare >= 0 && (emptySquares & (1ULL << targetSquare))) {
                if (moveCount < maxMoves) {
                    moveList[moveCount++] = BitMove(square, targetSquare, Pawn);
                }

                if (rank == 6) {
                    int doublePushSquare = square - 16;
                    if (emptySquares & (1ULL << doublePushSquare)) {
                        if (moveCount < maxMoves) {
                            moveList[moveCount++] = BitMove(square, doublePushSquare, Pawn);
                        }
                    }
                }
            }
            
            // diagonal captures
            if (file > 0) { // capture to the left
                int captureSquare = square - 9;
                if (captureSquare >= 0 && (enemyPieces & (1ULL << captureSquare))) {
                    if (moveCount < maxMoves) {
                        moveList[moveCount++] = BitMove(square, captureSquare, Pawn);
                    }
                }
            }
            if (file < 7) { //capture to the right
                int captureSquare = square - 7;
                if (captureSquare >= 0 && (enemyPieces & (1ULL << captureSquare))) {
                    if (moveCount < maxMoves) {
                        moveList[moveCount++] = BitMove(square, captureSquare, Pawn);
                    }
                }
            }
        }
    });
}

void Chess::generateKnightMoves(BitMove* moveList, int& moveCount, int maxMoves)
{
    int currentPlayer = getCurrentPlayer()->playerNumber();
    
    uint64_t bitboards[2][7];
    buildBitboards(bitboards);
    
    uint64_t knights = bitboards[currentPlayer][Knight];
    uint64_t friendlyPieces = 0ULL;
    
    for (int piece = Pawn; piece <= King; piece++) {
        friendlyPieces |= bitboards[currentPlayer][piece];
    }
    
    // KNIGHT PROCESSING
    BitboardElement knightBB(knights);
    knightBB.forEachBit([&](int square) {
        if (moveCount >= maxMoves) return;
        
        // Get all knight moves from this square
        uint64_t attacks = KnightAttacks[square];
        
        attacks &= ~friendlyPieces;

        BitboardElement attackBB(attacks);
        attackBB.forEachBit([&](int targetSquare) {
            if (moveCount < maxMoves) {
                moveList[moveCount++] = BitMove(square, targetSquare, Knight);
            }
        });
    });
}

void Chess::generateKingMoves(BitMove* moveList, int& moveCount, int maxMoves)
{
    int currentPlayer = getCurrentPlayer()->playerNumber();
    
    uint64_t bitboards[2][7];
    buildBitboards(bitboards);
    
    uint64_t king = bitboards[currentPlayer][King];
    uint64_t friendlyPieces = 0ULL;

    for (int piece = Pawn; piece <= King; piece++) {
        friendlyPieces |= bitboards[currentPlayer][piece];
    }
    
    // KING PROCESSING
    BitboardElement kingBB(king);
    kingBB.forEachBit([&](int square) {
        if (moveCount >= maxMoves) return;
        
        uint64_t attacks = KingAttacks[square];
        
        attacks &= ~friendlyPieces;
        
        BitboardElement attackBB(attacks);
        attackBB.forEachBit([&](int targetSquare) {
            if (moveCount < maxMoves) {
                moveList[moveCount++] = BitMove(square, targetSquare, King);
            }
        });
    });
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
