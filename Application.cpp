#include "Application.h"
#include "imgui/imgui.h"
#include "classes/TicTacToe.h"
#include "classes/Checkers.h"
#include "classes/Othello.h"
#include "classes/Chess.h"

namespace ClassGame {
        //
        // our global variables
        //
        Game *game = nullptr;
        bool gameOver = false;
        int gameWinner = -1;

        //
        // game starting point
        // this is called by the main render loop in main.cpp
        //
        void GameStartUp() 
        {
            game = nullptr;
        }

        //
        // game render loop
        // this is called by the main render loop in main.cpp
        //
        void RenderGame() 
        {
                ImGui::DockSpaceOverViewport();

                //ImGui::ShowDemoWindow();

                ImGui::Begin("Settings");

                if (gameOver) {
                    ImGui::Text("Winner: %d", gameWinner);
                    if (ImGui::Button("Reset Game")) {
                        game->stopGame();
                        game->setUpBoard();
                        gameOver = false;
                        gameWinner = -1;
                    }
                }
                if (!game) {
                    if (ImGui::Button("Start Tic-Tac-Toe")) {
                        game = new TicTacToe();
                        game->setUpBoard();
                    }
                    if (ImGui::Button("Start Checkers")) {
                        game = new Checkers();
                        game->setUpBoard();
                    }
                    if (ImGui::Button("Start Othello")) {
                        game = new Othello();
                        game->setUpBoard();
                    }
                    if (ImGui::Button("Start Chess")) {
                        game = new Chess();
                        game->setUpBoard();
                    }
                    if (ImGui::Button("Chess vs AI (Black)")) {
                        Chess* chess = new Chess();
                        chess->setAIPlayerChoice(1);
                        game = chess;
                        game->setUpBoard();
                    }
                    if (ImGui::Button("Chess vs AI (White)")) {
                        Chess* chess = new Chess();
                        chess->setAIPlayerChoice(0);
                        game = chess;
                        game->setUpBoard();
                    }
                } else {
                    ImGui::Text("Current Player Number: %d", game->getCurrentPlayer()->playerNumber());
                    std::string stateString = game->stateString();
                    int stride = game->_gameOptions.rowX;
                    int height = game->_gameOptions.rowY;

                    for(int y=0; y<height; y++) {
                        ImGui::Text("%s", stateString.substr(y*stride,stride).c_str());
                    }
                    ImGui::Text("Current Board State: %s", game->stateString().c_str());
                    
                    // Show first 20 moves the array can hold, legal moves
                    Chess* chess = dynamic_cast<Chess*>(game);
                    if (chess && ImGui::Button("Test Moves")) {
                        chess->testMoveGeneration();
                    }

                    if (chess && ImGui::CollapsingHeader("Chess Test Panel")) {
                        if (ImGui::Button("Reset")) {
                            chess->loadPositionFromFEN("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
                        }
                        if (ImGui::Button("Castling Test")) {
                            chess->loadPositionFromFEN("r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1");
                        }
                        if (ImGui::Button("Enpassant Test")) {
                            chess->loadPositionFromFEN("4k3/8/8/3pP3/8/8/8/4K3 w - d6 0 1");
                        }
                        if (ImGui::Button("Promotion Test")) {
                            chess->loadPositionFromFEN("4k3/P7/8/8/8/8/7p/4K3 w - - 0 1");
                        }
                        if (ImGui::Button("King Capture Test")) {
                            chess->loadPositionFromFEN("4k3/8/8/8/8/8/4Q3/4K3 w - - 0 1");
                        }
                    }
                }                ImGui::End();

                ImGui::Begin("GameWindow", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize);
                if (game) {
                    if (game->gameHasAI() && (game->getCurrentPlayer()->isAIPlayer() || game->_gameOptions.AIvsAI))
                    {
                        game->updateAI();
                    }
                    game->drawFrame();
                }
                ImGui::End();
        }

        //
        // end turn is called by the game code at the end of each turn
        // this is where we check for a winner
        //
        void EndOfTurn() 
        {
            Player *winner = game->checkForWinner();
            if (winner)
            {
                gameOver = true;
                gameWinner = winner->playerNumber();
            }
            if (game->checkForDraw()) {
                gameOver = true;
                gameWinner = -1;
            }
        }
}
