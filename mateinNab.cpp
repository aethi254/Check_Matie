#include <iostream>
#include "chess.hpp"
#include <vector>
#include <limits>
using namespace std;
using namespace chess;

const int INF = 100000;

bool isCheckmate(Board &board) {
    Movelist ml;
    movegen::legalmoves<>(ml, board);
    return ml.empty() && board.inCheck();
}

int mateinN(Board &board, int depth, Color attacker, vector<Move> &moves, int alpha, int beta) {
    if (depth == 0) {
        if (isCheckmate(board) && board.sideToMove() != attacker) {
            return INF;
        }
        return -INF;
    }

    Movelist ml;
    movegen::legalmoves<>(ml, board);
    if (ml.empty()) {
        if (isCheckmate(board) && board.sideToMove() != attacker) {
            return INF;
        }
        return -INF;
    }

    Color side = board.sideToMove();
    if (side == attacker) {
        int best = -INF;
        for (auto m : ml) {
            board.makeMove(m);
            vector<Move> nextLine;
            int score = mateinN(board, depth - 1, attacker, nextLine, alpha, beta);
            board.unmakeMove(m);
            if (score >= best) {
                best = score;
                moves.clear();
                moves.push_back(m);
                moves.insert(moves.end(), nextLine.begin(), nextLine.end());
            }
            alpha = max(alpha, best);
            if (beta <= alpha) break;
        }
        return best;
    } else {
        int worst = INF;
        for (auto m : ml) {
            board.makeMove(m);
            vector<Move> nextLine;
            int score = mateinN(board, depth - 1, attacker, nextLine, alpha, beta);
            board.unmakeMove(m);
            if (score <= worst) {
                worst = score;
                moves.clear();
                moves.push_back(m);
                moves.insert(moves.end(), nextLine.begin(), nextLine.end());
            }
            beta = min(beta, worst);
            if (beta <= alpha) break;
        }
        return worst;
    }
}

int main() {
    int N;
    string fen;
    cin >> N;
    int depth = 2*N;
    cin.ignore(numeric_limits<streamsize>::max(), '\n');
    getline(cin, fen);

    Board board;
    board.setFen(fen);
    vector<Move> moves;
    Color attacker = board.sideToMove();
    int score = mateinN(board, depth, attacker, moves, -INF, INF);
    if (score > 0) {
        cout << "Checkmate in " << N << " moves found";
        if (!moves.empty())
            cout << " " << uci::moveToUci(moves[0]);
    } else {
        cout << "No checkmate in " << N << " moves found";
    }
    return 0;
}