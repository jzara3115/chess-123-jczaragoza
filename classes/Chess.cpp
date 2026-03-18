#include "Chess.h"
#include "Evaluate.h"
#include <limits>
#include <cmath>
#include <iostream>
#include <algorithm>
#include <cctype>

Chess::Chess()
{
    _grid = new Grid(8, 8);
    _aiPlayerChoice = -1;
    _searchDepth = 3;
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
    if (_aiPlayerChoice == 0 || _aiPlayerChoice == 1) {
        setAIPlayer(_aiPlayerChoice);
    }
    _gameOptions.rowX = 8;
    _gameOptions.rowY = 8;

    _grid->initializeChessSquares(pieceSize, "boardsquare.png");
    FENtoBoard("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR");

    startGame();
}

void Chess::setAIPlayerChoice(int playerNumber)
{
    _aiPlayerChoice = playerNumber;
}

bool Chess::gameHasAI()
{
    return _gameOptions.AIPlaying;
}

void Chess::updateAI()
{
    int currentPlayer = getCurrentPlayer()->playerNumber();
    if (!_gameOptions.AIPlaying || !getCurrentPlayer()->isAIPlayer()) {
        return;
    }

    std::vector<BitMove> moves = generateAllMovesForPlayer(currentPlayer);
    if (moves.empty()) {
        endTurn();
        return;
    }

    BitMove bestMove = findBestMove(_searchDepth, currentPlayer);
    if (applyMoveOnBoard(bestMove)) {
        endTurn();
    }
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
    return generateAllMovesForPlayer(getCurrentPlayer()->playerNumber());
}

std::vector<BitMove> Chess::generateAllMovesForPlayer(int currentPlayer)
{
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

int Chess::pieceSquareScore(ChessPiece piece, int square, int playerNumber) const
{
    int file = square % 8;
    int rank = square / 8;
    int index = (playerNumber == 0) ? square : ((7 - rank) * 8 + file);

    switch (piece) {
        case Pawn: return pawnTable[index];
        case Knight: return knightTable[index];
        case Bishop: return bishopTable[index];
        case Rook: return rookTable[index];
        case Queen: return queenTable[index];
        case King: return kingTable[index];
        default: return 0;
    }
}

int Chess::evaluateBoard() const
{
    const int pieceValues[7] = { 0, 100, 320, 330, 500, 900, 20000 };
    int score = 0;

    _grid->forEachSquare([&](ChessSquare* square, int x, int y) {
        Bit* bit = square->bit();
        if (!bit) {
            return;
        }

        int player = (bit->gameTag() & 128) ? 1 : 0;
        ChessPiece piece = (ChessPiece)(bit->gameTag() & 0x7F);
        int boardSquare = y * 8 + x;
        int pieceScore = pieceValues[piece] + pieceSquareScore(piece, boardSquare, player);

        score += (player == 0) ? pieceScore : -pieceScore;
    });

    return score;
}

int Chess::evaluateState(const std::string& state) const
{
    const int pieceValues[7] = { 0, 100, 320, 330, 500, 900, 20000 };
    int score = 0;

    for (int i = 0; i < 64 && i < (int)state.size(); i++) {
        char c = state[i];
        if (c == '0') {
            continue;
        }

        int player = (c >= 'A' && c <= 'Z') ? 0 : 1;
        ChessPiece piece = NoPiece;
        switch ((char)std::toupper(c)) {
            case 'P': piece = Pawn; break;
            case 'N': piece = Knight; break;
            case 'B': piece = Bishop; break;
            case 'R': piece = Rook; break;
            case 'Q': piece = Queen; break;
            case 'K': piece = King; break;
            default: piece = NoPiece; break;
        }

        if (piece == NoPiece) {
            continue;
        }

        int pieceScore = pieceValues[piece] + pieceSquareScore(piece, i, player);
        score += (player == 0) ? pieceScore : -pieceScore;
    }

    return score;
}

int Chess::evaluateForPlayer(int playerNumber) const
{
    int whitePerspective = evaluateBoard();
    return playerNumber == 0 ? whitePerspective : -whitePerspective;
}

int Chess::evaluateStateForPlayer(const std::string& state, int playerNumber) const
{
    int whitePerspective = evaluateState(state);
    return playerNumber == 0 ? whitePerspective : -whitePerspective;
}

bool Chess::applyMoveOnBoard(const BitMove& move)
{
    int fromX = move.from % 8;
    int fromY = move.from / 8;
    int toX = move.to % 8;
    int toY = move.to / 8;

    ChessSquare* fromSquare = _grid->getSquare(fromX, fromY);
    ChessSquare* toSquare = _grid->getSquare(toX, toY);
    if (!fromSquare || !toSquare) {
        return false;
    }

    Bit* movingBit = fromSquare->bit();
    if (!movingBit) {
        return false;
    }

    // Move destination first so the source holder does not delete the moving piece.
    // BitHolder::setBit(nullptr) deletes its current bit, so clearing source first is unsafe here.
    toSquare->setBit(movingBit);
    movingBit->setParent(toSquare);
    movingBit->moveTo(toSquare->getPosition());

    // Ensure source holder releases stale pointer to moved piece.
    fromSquare->bit();

    return true;
}

std::string Chess::applyMoveToState(const std::string& state, const BitMove& move) const
{
    std::string next = state;
    if (move.from < next.size() && move.to < next.size()) {
        next[move.to] = next[move.from];
        next[move.from] = '0';
    }
    return next;
}

std::vector<BitMove> Chess::generateAllMovesForPlayerFromState(const std::string& state, int currentPlayer) const
{
    std::vector<BitMove> moves;
    moves.reserve(256);

    auto isFriendly = [&](char p) {
        if (p == '0') return false;
        return currentPlayer == 0 ? (p >= 'A' && p <= 'Z') : (p >= 'a' && p <= 'z');
    };
    auto isEnemy = [&](char p) {
        if (p == '0') return false;
        return currentPlayer == 0 ? (p >= 'a' && p <= 'z') : (p >= 'A' && p <= 'Z');
    };

    for (int from = 0; from < 64 && from < (int)state.size(); from++) {
        char pieceChar = state[from];
        if (!isFriendly(pieceChar)) {
            continue;
        }

        int file = from % 8;
        int rank = from / 8;
        ChessPiece piece = NoPiece;
        switch ((char)std::toupper(pieceChar)) {
            case 'P': piece = Pawn; break;
            case 'N': piece = Knight; break;
            case 'B': piece = Bishop; break;
            case 'R': piece = Rook; break;
            case 'Q': piece = Queen; break;
            case 'K': piece = King; break;
            default: piece = NoPiece; break;
        }

        if (piece == Pawn) {
            if (currentPlayer == 0) {
                int one = from + 8;
                if (one < 64 && state[one] == '0') {
                    moves.push_back(BitMove(from, one, Pawn));
                    int two = from + 16;
                    if (rank == 1 && two < 64 && state[two] == '0') {
                        moves.push_back(BitMove(from, two, Pawn));
                    }
                }
                int capL = from + 7;
                int capR = from + 9;
                if (file > 0 && capL < 64 && isEnemy(state[capL])) {
                    moves.push_back(BitMove(from, capL, Pawn));
                }
                if (file < 7 && capR < 64 && isEnemy(state[capR])) {
                    moves.push_back(BitMove(from, capR, Pawn));
                }
            }
            else {
                int one = from - 8;
                if (one >= 0 && state[one] == '0') {
                    moves.push_back(BitMove(from, one, Pawn));
                    int two = from - 16;
                    if (rank == 6 && two >= 0 && state[two] == '0') {
                        moves.push_back(BitMove(from, two, Pawn));
                    }
                }
                int capL = from - 9;
                int capR = from - 7;
                if (file > 0 && capL >= 0 && isEnemy(state[capL])) {
                    moves.push_back(BitMove(from, capL, Pawn));
                }
                if (file < 7 && capR >= 0 && isEnemy(state[capR])) {
                    moves.push_back(BitMove(from, capR, Pawn));
                }
            }
        }
        else if (piece == Knight) {
            uint64_t attacks = KnightAttacks[from];
            BitboardElement attackBB(attacks);
            attackBB.forEachBit([&](int to) {
                if (to >= 0 && to < 64 && !isFriendly(state[to])) {
                    moves.push_back(BitMove(from, to, Knight));
                }
            });
        }
        else if (piece == King) {
            uint64_t attacks = KingAttacks[from];
            BitboardElement attackBB(attacks);
            attackBB.forEachBit([&](int to) {
                if (to >= 0 && to < 64 && !isFriendly(state[to])) {
                    moves.push_back(BitMove(from, to, King));
                }
            });
        }
        else {
            static const int rookDirs[4][2] = { {1,0}, {-1,0}, {0,1}, {0,-1} };
            static const int bishopDirs[4][2] = { {1,1}, {1,-1}, {-1,1}, {-1,-1} };
            static const int queenDirs[8][2] = {
                {1,0}, {-1,0}, {0,1}, {0,-1}, {1,1}, {1,-1}, {-1,1}, {-1,-1}
            };

            const int (*dirs)[2] = nullptr;
            int dirCount = 0;
            if (piece == Rook) {
                dirs = rookDirs;
                dirCount = 4;
            }
            else if (piece == Bishop) {
                dirs = bishopDirs;
                dirCount = 4;
            }
            else if (piece == Queen) {
                dirs = queenDirs;
                dirCount = 8;
            }

            for (int d = 0; d < dirCount; d++) {
                int x = file + dirs[d][0];
                int y = rank + dirs[d][1];
                while (x >= 0 && x < 8 && y >= 0 && y < 8) {
                    int to = y * 8 + x;
                    if (isFriendly(state[to])) {
                        break;
                    }
                    moves.push_back(BitMove(from, to, piece));
                    if (isEnemy(state[to])) {
                        break;
                    }
                    x += dirs[d][0];
                    y += dirs[d][1];
                }
            }
        }
    }

    return moves;
}

int Chess::negamax(const std::string& state, int depth, int alpha, int beta, int playerNumber)
{
    if (depth == 0) {
        return evaluateStateForPlayer(state, playerNumber);
    }

    std::vector<BitMove> moves = generateAllMovesForPlayerFromState(state, playerNumber);
    if (moves.empty()) {
        return evaluateStateForPlayer(state, playerNumber);
    }

    int bestScore = std::numeric_limits<int>::min() / 2;
    for (const BitMove& move : moves) {
        std::string nextState = applyMoveToState(state, move);
        int score = -negamax(nextState, depth - 1, -beta, -alpha, 1 - playerNumber);

        if (score > bestScore) {
            bestScore = score;
        }
        if (score > alpha) {
            alpha = score;
        }
        if (alpha >= beta) {
            break;
        }
    }

    return bestScore;
}

BitMove Chess::findBestMove(int depth, int playerNumber)
{
    std::string rootState = stateString();
    std::vector<BitMove> moves = generateAllMovesForPlayerFromState(rootState, playerNumber);
    if (moves.empty()) {
        return BitMove();
    }

    int alpha = std::numeric_limits<int>::min() / 2;
    int beta = std::numeric_limits<int>::max() / 2;
    int bestScore = std::numeric_limits<int>::min() / 2;
    BitMove bestMove = moves[0];

    for (const BitMove& move : moves) {
        std::string nextState = applyMoveToState(rootState, move);
        int score = -negamax(nextState, depth - 1, -beta, -alpha, 1 - playerNumber);

        if (score > bestScore) {
            bestScore = score;
            bestMove = move;
        }
        if (score > alpha) {
            alpha = score;
        }
    }

    return bestMove;
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
        char pieceChar = (index < (int)s.size()) ? s[index] : '0';
        if (pieceChar == '0') {
            square->setBit(nullptr);
            return;
        }

        int playerNumber = (pieceChar >= 'A' && pieceChar <= 'Z') ? 0 : 1;
        ChessPiece pieceType = NoPiece;
        switch ((char)std::toupper(pieceChar)) {
            case 'P': pieceType = Pawn; break;
            case 'N': pieceType = Knight; break;
            case 'B': pieceType = Bishop; break;
            case 'R': pieceType = Rook; break;
            case 'Q': pieceType = Queen; break;
            case 'K': pieceType = King; break;
            default: pieceType = NoPiece; break;
        }

        if (pieceType == NoPiece) {
            square->setBit(nullptr);
            return;
        }

        Bit* piece = PieceForPlayer(playerNumber, pieceType);
        piece->setGameTag(pieceType + (playerNumber * 128));
        square->setBit(piece);
        piece->setParent(square);
        piece->moveTo(square->getPosition());
    });
}
