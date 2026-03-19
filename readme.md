Jc Zaragoza
1983950
CMPM 123
3/2/26

Update (3/18/26)

I focused on adding the special moves. pawn promotion, en passant, and castling. I also added a end condition where when the king is taken, the game is officially over and no player can move anymore. This was important because it made the game flow line up better with the assignment goals and made testing easier to reason about.

For promotion, I implemented automatic pawn promotion to a queen when a pawn reaches the back rank. This works for both white and black. For en passant, I added tracking for an enpassant target square after a two-square pawn move. Then in pawn move generation I added the enpassant capture move only when that square is available, and in move resolution I remove the captured pawn from the correct adjacent square. For castling, I added castling rights tracking for both sides and both directions (white king side/queen side and black king side/queen side). King move generation now adds castling moves when the path is clear and rights still exist. Then during move application, if the king castles, the rook is moved automatically to the correct square. I also updated castling rights when a king moves, when a rook moves, and when a rook is taken on a square the rook started at.

The main challenge here was keeping these rules for both human drag/drop moves and AI-generated moves. To solve that, I made side effects (promotion, en passant capture cleanup, castling rook shift, rights updates, king-capture winner lock) into shared post move logic so both paths can follow the same rules.

I also added a Chess Test Panel in the UI with one-click FEN scenario buttons for castling, en passant, promotion, and king-capture board placements. That made debugging and validating edge cases way faster than manually setting up positions move-by-move.

Update (3/16/26) - Negamax AI with Alpha/Beta

For this milestone, I added a chess AI using the Negamax search with alpha/beta pruning. The AI is set to search to a depth of 3 plies (_searchDepth = 3), and I made it flexible so it can play as White or Black depending on the mode. The main goal was to integrate a real decision-making loop into the existing chess framework without breaking the turn-based gameplay flow or the move validation system.

To support search properly, I extended the move generation system so it could operate independently from UI turn handling. I added a player-specific move generator, generateAllMovesForPlayer(int playerNumber), which lets the AI generate moves for either side at any point in the search tree. I also added negamax(depth, alpha, beta, playerNumber) as the core recursive algorithm, using alpha/beta cutoffs to prune branches that canr improve the result. Then I wrote findBestMove(depth, playerNumber) to evaluate all candidate moves and select the one with the highest negamax score.

To make the search powerful enough, I added an evaluateBoard() function based on a combination of material scoring and piece-square tables, using values from Evaluate.h. This gave the AI a basic sense of poisitioning instead of only valuing captures. For gameplay, I added updateAI() and gameHasAI() in Chess so the AI automatically makes a move when it is the AI-controlled side’s turn. I also added UI start options so the game can be launched in three modes, human vs human, AI as Black, or AI as White.

The biggest challenge was state restoration during recursive search. Since the AI needs to make moves, evaluate future positions, and then undo those moves, I needed a good way to restore the board without leaving corrupted pieces. I solved this by improving setStateString() so it fully rebuilds chess pieces (type, owner, tag, and position) from the board notation string. With that in place, the search can safely change the board state, recurse, and return back up the tree without errors or ghost pieces.

At depth 3, the AI plays legally and can make reasonable choices, like capturing hanging pieces and responding in a lot of opening and middlegame positions. But its strength is still limited by the current rule scope and depth.

Update (3/9/26)

A big part of this update was adding the required generateAllMoves() method, which gives a std::vector<BitMove> containing every legal move for the side to move. This gave me a clean interface for move generation while also making it easier to test and work with move lists. The flow stayed the same conceptually, I first build bitboards for both sides from the live board state, then make moves piece by piece for the player (pawn, knight, bishop, rook, queen, king), and then return the full list of legal destinations with generateAllMoves().

For the sliding pieces (rooks, bishops, and queens), the main challenge was adding directional movement and making sure blocking and capturing was like real chess. Rooks generate moves in the four directions, bishops generate moves in the four diagonal directions, and queens combine both patterns to move eight directions. In every case, movement continues square-by-square until it is blocked by a piece. Friendly pieces stop movement immediately and cant be captured, while enemy pieces can be captured but also stop the piece from moving in that direction.

Even though I introduced generateAllMoves() as the main move generation method, I kept the existing generateMoves(BitMove* moveList, int maxMoves) interface as a compatibility wrapper. This was important because the original array based approach is still good for debugging, breakpoints, and the screenshot requirements. The wrapper calls generateAllMoves() and then copies the results into the move array, keeping the behavior the same while allowing the new system to be the source of truth.

With these changes, movement is fully turn-based for white and black and supports legal capture behavior for all current pieces. This update has the standard movement ruleset and also sets up a solid foundation for adding special moves or more rule enforcement later if I wanted to add things like that.

After the board setup assignment, the goal was implementing a movement system. Since chess move generation can get really hard really fast, I focused on building a foundation that could scale later. To do this, I added a bitboard-based representation of the board, which uses a 64-bit integer to represent piece positions and allows move generation to be done more efficiently through bit operations instead of constantly looping through a grid.

I created a new Bitboard.h header to hold the core data structures. This had a BitMove structure for moves, helper functions for scanning bits, and pre calculated attack tables for Knights and Kings. The pre calculated tables made the move generation easier, since instead of recalculating movement every time, I could just reference a lookup table based on the piece’s square and then filter out illegal destinations.

Then, I implemented legal move generation for the pawns, knights, and kings. Pawns needed the most special case logic because their movement rules depend on direction and whether they are moving or capturing. I did single forward movement, double pushes from the starting rank, and diagonal captures, with separate logic for white and black so each side advances in the correct direction. Knights were more straightforward because their move pattern does not depend on blockers, and their attacks can be generated purely from the pre-calculated bitboards. Kings were like knights in terms of attack table usage, but needed more filtering so they couldnt move onto friendly squares.

After adding move generation, I updated canBitMoveFromTo() so it can detect moves based on the legal move list instead of relying on ad-hoc checks. The process is to find all legal moves for the current player, compare the requested move against that list, and only allow it if it matches a legal entry. This made the validation system much more consistent, since every move the player attempts is checked against the same rules the engine uses to generate moves.

I also added a turn-based playstyle where players change between White and Black, and only pieces belonging to the current player can be moved. This prevented out of turn moves and kept the logic aligned with real chess rules even with only a partial piece set implemented. Then I implemented capture mechanics so pieces can properly take enemy pieces. Captures follow normal chess rules, a piece can move onto an enemy-occupied square and remove that piece from the board, but friendly pieces cannot be captured. Pawns required extra care here because their capture rules are different from their normal forward movement, so I made sure diagonal captures were only allowed when an enemy piece exists on the target square. To make sure that move generation was working, I added a testMoveGeneration() function that runs during board setup. This function finds all legal moves from the current position, and prints the first 20 moves to the console.

I used classes/ Bitboard.h as the main place for bitboard structures and attack tables, then updated Chess.h and Chess.cpp to include move generation, FEN parsing, and validation logic for cleaner code. The key functions I built around were FENtoBoard() for setup, buildBitboards() for converting the grid representation into bitboards, generateMoves() and the piece-specific generators for creating the move list, and canBitMoveFromTo() for enforcing legality.

For the overall strategy, I based my approach on bitboard-style chess programming patterns. The flow is basically build bitboards, identify all pieces for the current player, generate legal moves using bit operations and attack tables, filter out moves that land on friendly pieces, and return the list of legal moves.