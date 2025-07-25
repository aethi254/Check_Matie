#include <iostream>
#include "chess.hpp"
using namespace std;
using namespace chess;
bool isCheckmate(Board &board) {
    Movelist ml;
    movegen::legalmoves<>(ml, board);
    if (ml.size() == 0 && board.inCheck()) return true;
    return false;
}
bool hasMateInOne(Board &board) {
    Movelist ml;
    movegen::legalmoves<>(ml, board);
    for (Move m : ml) {
        board.makeMove(m);
        if (isCheckmate(board)) {
            board.unmakeMove(m);
            return true;
        }
        board.unmakeMove(m);
    }
    return false;
}
bool findMateInTwoMove(Board &board, Move &outMove) {
    Color stm = board.sideToMove();
    Movelist firstMoves;
    movegen::legalmoves<>(firstMoves, board);
    for (Move m1 : firstMoves) {
        board.makeMove(m1);
        bool allRepliesLose = true;
        Movelist replies;
        movegen::legalmoves<>(replies, board);
        if (replies.empty()) {
            if (isCheckmate(board)) {
                board.unmakeMove(m1);
                outMove = m1;
                return true;
            } else {
                allRepliesLose = false;
            }
        } else {
            for (Move reply : replies) {
                board.makeMove(reply);
                if (!hasMateInOne(board)) {
                    allRepliesLose = false;
                    board.unmakeMove(reply);
                    break;
                }
                board.unmakeMove(reply);
            }
        }
        board.unmakeMove(m1);
        if (allRepliesLose) {
            outMove = m1;
            return true;
        }
    }
    return false;
}
int main() {
    string fen;
    cout<<"Enter the starting position";
    getline(cin,fen);
    Board board;
    board.setFen(fen);
    Move mateMove;
    if (findMateInTwoMove(board, mateMove)) {
        std::cout << "Mate in two found: " << uci::moveToUci(mateMove) << "\n";
    } 
    else {
        std::cout << "No forced mate in two from this position.\n";
    }
    return 0;
}