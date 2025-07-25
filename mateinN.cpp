#include <iostream>
#include "chess.hpp"
#include <vector>
using namespace std;
using namespace chess;

bool isCheckmate(Board &board) {
    Movelist ml;
    movegen::legalmoves<>(ml, board);
    return ml.empty() && board.inCheck();
}

bool mateinN(Board &board, int N, Color attacker, vector<Move> &moves) {
    if (N == 0)
        return isCheckmate(board) && board.sideToMove() != attacker;

    Movelist ml;
    movegen::legalmoves<>(ml, board);
    if (ml.empty())
        return isCheckmate(board) && board.sideToMove() != attacker;

    Color side = board.sideToMove();
    if (side == attacker) {
        for (auto m : ml) {
            board.makeMove(m);
            vector<Move> nextLine;
            bool win = mateinN(board, N - 1, attacker, nextLine);
            board.unmakeMove(m);
            if (win) {
                moves.clear();
                moves.push_back(m);
                moves.insert(moves.end(), nextLine.begin(), nextLine.end());
                return true;
            }
        }
        return false;
    }
    for (auto m : ml) {
        board.makeMove(m);
        bool win = mateinN(board, N - 1, attacker, moves);
        board.unmakeMove(m);
        if (!win)
            return false;
    }
    return true;
}

int main() {
    int N;
    string fen;
    cin >> N;
    getline(cin, fen);

    Board board;
    board.setFen(fen);
    vector<Move> moves;
    Color attacker = board.sideToMove();

    if (mateinN(board, N, attacker, moves)) {
        cout << "Mate in " << N << " found:";
        for (auto &m : moves)
            cout << " " << uci::moveToUci(m);
    } else {
        cout << "No forced mate in " << N << " found";
    }
    return 0;
}