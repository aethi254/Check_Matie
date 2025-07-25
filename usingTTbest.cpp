#include<iostream>
#include "chess.hpp"
#include<vector>
#include<sstream>
#include<limits>
#include<unordered_map>
using namespace std;
using namespace chess;

const int INF = std::numeric_limits<int>::max() / 2;

struct TranspositionEntry {
    uint64_t positionHash;    
    int searchDepth;         
    int evaluationScore;     
    Move bestMoveFound;       
    int boundType;            
    
    TranspositionEntry() {
        positionHash = 0;
        searchDepth = -1;
        evaluationScore = 0;
        bestMoveFound = Move::NULL_MOVE;
        boundType = 0;
    }
    
    TranspositionEntry(uint64_t hash, int depth, int score, Move move, int bound) {
        positionHash = hash;
        searchDepth = depth;
        evaluationScore = score;
        bestMoveFound = move;
        boundType = bound;
    }
};

unordered_map<uint64_t, TranspositionEntry> hashTable;

void clearHashTable() {
    hashTable.clear();
}

bool isCheckmate(Board &board) 
{
    Movelist ml;
    movegen::legalmoves<>(ml, board);
    return ml.empty() && board.inCheck();
}

bool isStalemate(Board &board)
{
    Movelist ml;
    movegen::legalmoves<>(ml,board);
    return ml.empty() && !board.inCheck();
}

int getpieceValue(PieceType piece)
{
    if (piece == PieceType::PAWN) return 100;
    if (piece == PieceType::KNIGHT) return 320;
    if (piece == PieceType::BISHOP) return 330;
    if (piece == PieceType::ROOK) return 500;
    if (piece == PieceType::QUEEN) return 900;
    if (piece == PieceType::KING) return 20000;
    return 0;
}

PieceType getcapturedPiece(const Board &board, const Move &move)
{
    return board.at<PieceType>(move.to());
}

int evaluateBoard(Board &board) 
{
    if (isCheckmate(board)) 
    {
        return -20000;
    }
    else if (isStalemate(board))
    {
        return 0;
    }
    
    int score = 0;
    
    for (PieceType pt : {PieceType::PAWN, PieceType::KNIGHT, PieceType::BISHOP, PieceType::ROOK, PieceType::QUEEN, PieceType::KING}) 
    {
        Bitboard wBB = board.pieces(pt, Color::WHITE);
        Bitboard bBB = board.pieces(pt, Color::BLACK);
        int wCount = wBB.count();
        int bCount = bBB.count();   
        score += getpieceValue(pt) * (wCount - bCount);
    }

    array<Square,4> center = { Square("d4"),Square("d5"),Square("e4"),Square("e5") };
    array<Square,8> centerControl = { Square("c3"),Square("d3"),Square("e3"),Square("f3"),
                                     Square("c6"),Square("d6"),Square("e6"),Square("f6") };
    
    for (auto sq: center) {
        Piece p = board.at(sq);
        if (p != Piece::NONE) {
            if (p.color() == Color::WHITE) score += 30;
            else if (p.color() == Color::BLACK) score -= 30;
        }
    }
    
    for (auto sq: centerControl) {
        Piece p = board.at(sq);
        if (p.type() == PieceType::PAWN) {
            if (p.color() == Color::WHITE) score += 15;
            else if (p.color() == Color::BLACK) score -= 15;
        }
    }
    
    Bitboard wp = board.pieces(PieceType::PAWN, Color::WHITE);
    while (wp) 
    {
        Square sq = wp.pop();
        int rank = sq.rank();
        int file = sq.file();
        score += 3 * rank;
        if (file == 3 || file == 4) {
            score += 5 * rank;
            if (rank == 3) score += 10;
            if (rank == 4) score += 15;
        }
    }
    
    Bitboard bp = board.pieces(PieceType::PAWN, Color::BLACK);
    while (bp) 
    {
        Square sq = bp.pop();
        int rank = sq.rank();
        int file = sq.file();
        score -= 3 * (7 - rank);
        if (file == 3 || file == 4) {
            score -= 5 * (7 - rank);
            if (rank == 4) score -= 10;
            if (rank == 3) score -= 15;
        }
    }
    
    for (Color c : {Color::WHITE, Color::BLACK}) 
    {
        Bitboard king = board.pieces(PieceType::KING, c);
        if (king) {
            Square kingSq = king.pop();
            int kingRank = kingSq.rank();
            int kingFile = kingSq.file();
            
            int shield = 0;
            int shieldRank = kingRank + (c == Color::WHITE ? 1 : -1);
            
            if (shieldRank >= 0 && shieldRank <= 7) {
                for (int a = -1; a <= 1; ++a) {
                    int shieldFile = kingFile + a;
                    if (shieldFile >= 0 && shieldFile <= 7) {
                        Square shieldSq = Square(static_cast<File>(shieldFile), static_cast<Rank>(shieldRank));
                        Piece p = board.at(shieldSq);
                        if (p.type() == PieceType::PAWN && p.color() == c) {
                            shield++;
                        }
                    }
                }
            }
            score += (c == Color::WHITE ? 1 : -1) * shield * 10;
        }
    }
    
    Bitboard bB = board.pieces(PieceType::BISHOP, Color::BLACK);
    Bitboard wB = board.pieces(PieceType::BISHOP, Color::WHITE);
    int bCount = bB.count();
    int wCount = wB.count();
    if (wCount >= 2) score += 50;
    if (bCount >= 2) score -= 50;

    return board.sideToMove() == Color::WHITE ? score : -score;
}

int quiesce(Board &board, int alpha, int beta) 
{
    int stand = evaluateBoard(board);
    if (stand >= beta) return beta;
    if (stand > alpha) alpha = stand;
    
    Movelist captures;
    movegen::legalmoves<movegen::MoveGenType::CAPTURE>(captures, board);
    Movelist quiets;
    movegen::legalmoves<movegen::MoveGenType::QUIET>(quiets, board);
    for (auto &m : quiets) {
        PieceType piece = board.at<PieceType>(m.from());
        if (piece == PieceType::PAWN) {
            int toRank = m.to().rank();
            if (toRank == 7 || toRank == 0) {
                captures.add(m);
            }
        }
    }
    for (int i = 0; i < (int)captures.size(); ++i) {
        int best = i;
        int bestValue = -INF;
        
        for (int j = i; j < (int)captures.size(); ++j) {
            int victimValue = 0;
            int attackerValue = 0;
            if (board.at(captures[j].to()) != Piece::NONE) {
                victimValue = getpieceValue(getcapturedPiece(board, captures[j]));
                attackerValue = getpieceValue(board.at<PieceType>(captures[j].from()));
            }
            else if (captures[j].from().file() != captures[j].to().file() && 
                     board.at<PieceType>(captures[j].from()) == PieceType::PAWN) {
                victimValue = getpieceValue(PieceType::PAWN);
                attackerValue = getpieceValue(PieceType::PAWN);
            }
            else {
                PieceType fromPiece = board.at<PieceType>(captures[j].from());
                if (fromPiece == PieceType::PAWN) {
                    victimValue = getpieceValue(PieceType::QUEEN) - getpieceValue(PieceType::PAWN);
                    attackerValue = 0;
                }
            }
            
            int value = victimValue * 10 - attackerValue;
            
            if (value > bestValue) {
                bestValue = value;
                best = j;
            }
        }
        
        if (best != i) {
            swap(captures[i], captures[best]);
        }
    }
    
    for (auto &m : captures) {
        board.makeMove(m);
        int score = -quiesce(board, -beta, -alpha);
        board.unmakeMove(m);
        
        if (score >= beta) return beta;
        if (score > alpha) alpha = score;
    }
    
    return alpha;
}

void orderMoves(const Board &board, Movelist &ordered, const Move &bestfromprev = Move::NULL_MOVE) 
{
    ordered.clear();
    
    Movelist captures;
    movegen::legalmoves<movegen::MoveGenType::CAPTURE>(captures, board);
    
    Movelist quiets;
    movegen::legalmoves<movegen::MoveGenType::QUIET>(quiets, board);
    
    Movelist promotions;
    Movelist castles;
    Movelist others;

    if (bestfromprev != Move::NULL_MOVE)
    {
        ordered.add(bestfromprev);
    }
    
    for (auto &m : quiets) {
        PieceType piece = board.at<PieceType>(m.from());
        if (piece == PieceType::PAWN) {
            int toRank = m.to().rank();
            int fromRank = m.from().rank();
            if (toRank == 7 || toRank == 0) {
                promotions.add(m);
            } else {
                others.add(m);
            }
        } else if (m.from().file() != m.to().file() || abs(m.from().rank() - m.to().rank()) == 2) {
            if (piece == PieceType::KING && abs(m.from().file() - m.to().file()) == 2) {
                castles.add(m);
            } else {
                others.add(m);
            }
        } else {
            others.add(m);
        }
    }
    
    for (int i = 0; i < (int)captures.size(); ++i) {
        int best = i;
        int bestValue = -INF;
        
        for (int j = i; j < (int)captures.size(); ++j) {
            int victimValue = 0;
            int attackerValue = 0;
        
            if (board.at(captures[j].to()) != Piece::NONE) {
                victimValue = getpieceValue(getcapturedPiece(board, captures[j]));
                attackerValue = getpieceValue(board.at<PieceType>(captures[j].from()));
            }
            else if (captures[j].from().file() != captures[j].to().file() && 
                     board.at<PieceType>(captures[j].from()) == PieceType::PAWN) {
                victimValue = getpieceValue(PieceType::PAWN);
                attackerValue = getpieceValue(PieceType::PAWN);
            }
            
            int value = victimValue * 10 - attackerValue;
            
            if (value > bestValue) {
                bestValue = value;
                best = j;
            }
        }
        
        if (best != i) {
            swap(captures[i], captures[best]);
        }
    }

    for (auto &m : captures) 
    {
        if (bestfromprev == Move::NULL_MOVE || m != bestfromprev) {
            ordered.add(m);
        }
    }
    for (auto &m : promotions) 
    {
        if (bestfromprev == Move::NULL_MOVE || m != bestfromprev) {
            ordered.add(m);
        }
    }
    for (auto &m : castles) 
    {
        if (bestfromprev == Move::NULL_MOVE || m != bestfromprev) {
            ordered.add(m);
        }
    }
    for (auto &m : others) 
    {
        if (bestfromprev == Move::NULL_MOVE || m != bestfromprev) {
            ordered.add(m);
        }
    }
}

int alphaBeta(Board &board, int depth, int alpha, int beta, Move &bestMove, const Move &bestfromprev = Move::NULL_MOVE){
    
    uint64_t currentPositionHash = board.hash();
    
    Move hashMove = bestfromprev;
    
    if (hashTable.find(currentPositionHash) != hashTable.end()) {
        TranspositionEntry storedInfo = hashTable[currentPositionHash];
        
        if (storedInfo.searchDepth >= depth) {
            if (storedInfo.boundType == 0) {
                bestMove = storedInfo.bestMoveFound;
                return storedInfo.evaluationScore;
            }
            else if (storedInfo.boundType == 1 && storedInfo.evaluationScore >= beta) {
                bestMove = storedInfo.bestMoveFound;
                return beta;
            }
            else if (storedInfo.boundType == 2 && storedInfo.evaluationScore <= alpha) {
                bestMove = storedInfo.bestMoveFound;
                return alpha;
            }
        }

        if (storedInfo.bestMoveFound != Move::NULL_MOVE) {
            hashMove = storedInfo.bestMoveFound;
        }
    }
    
    if (isCheckmate(board)) {
        return -20000 + (6 - depth);
    }
    if (isStalemate(board)) {
        return 0;
    }
    
    if (depth == 0) {
        return quiesce(board, alpha, beta);
    }
    
    Movelist moves;
    orderMoves(board, moves, hashMove);

    if (moves.empty()) {
        return evaluateBoard(board);
    }
    
    bestMove = Move::NULL_MOVE;
    int originalAlpha = alpha;
    
    for (auto &m : moves) {
        board.makeMove(m);
        Move childBest;
        int score = -alphaBeta(board, depth - 1, -beta, -alpha, childBest);
        board.unmakeMove(m);
        
        if (score >= beta) {
            bestMove = m;
            TranspositionEntry newEntry(currentPositionHash, depth, beta, m, 1);
            hashTable[currentPositionHash] = newEntry;
            return beta;
        }
        if (score > alpha) {
            alpha = score;
            bestMove = m;
        }
    }

    if (bestMove == Move::NULL_MOVE && !moves.empty()) {
        bestMove = moves[0];
    }
    int boundType;
    if (alpha <= originalAlpha) {
        boundType = 2; 
    } else {
        boundType = 0; 
    }
    
    TranspositionEntry newEntry(currentPositionHash, depth, alpha, bestMove, boundType);
    hashTable[currentPositionHash] = newEntry;
    
    return alpha;
}

Move iterativedeep(Board &board, int maxdepth)
{
    Movelist rootMoves;
    movegen::legalmoves<>(rootMoves, board);
    
    if(rootMoves.empty()) {
        return Move::NULL_MOVE;
    }
    if(rootMoves.size() == 1) {
        return rootMoves[0];
    }
    
    Move bestMove = rootMoves[0];
    
    for (int depth = 1; depth <= maxdepth; depth++)
    {
        int bestScore = -INF;
        Move iterationBest = Move::NULL_MOVE;
        
        for (auto &move : rootMoves) {
            if (move == bestMove) {
                board.makeMove(move);
                Move dummy;
                int score = -alphaBeta(board, depth - 1, -INF, INF, dummy);
                board.unmakeMove(move);
                
                if (score > bestScore) {
                    bestScore = score;
                    iterationBest = move;
                }
                break;
            }
        }
        for (auto &move : rootMoves) {
            if (move != bestMove) {
                board.makeMove(move);
                Move dummy;
                int score = -alphaBeta(board, depth - 1, -INF, INF, dummy);
                board.unmakeMove(move);
                
                if (score > bestScore) {
                    bestScore = score;
                    iterationBest = move;
                }
            }
        }
        
        if (iterationBest != Move::NULL_MOVE) {
            bestMove = iterationBest;
        }
        if (abs(bestScore) > 19000) {
            break;
        }
    }
    return bestMove;
}

vector<string> splitString(const string& str, char delimiter) {
    vector<string> tokens;
    stringstream ss(str);
    string token;
    while (getline(ss, token, delimiter)) {
        if (!token.empty()) {
            tokens.push_back(token);
        }
    }
    return tokens;
}

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);
    std::cout.tie(nullptr);
    
    Board board;
    string line;
    
    while (getline(cin, line)) {
        size_t start = line.find_first_not_of(" \t\r\n");
        if (start == string::npos) continue;
        size_t end = line.find_last_not_of(" \t\r\n");
        line = line.substr(start, end - start + 1);
        
        if (line.empty()) continue;
        
        if (line == "uci") {
            std::cout << "id name Aethi" << endl;
            std::cout << "id author Atharva" << endl;
            std::cout << "uciok" << endl;
            std::cout.flush();
        }
        else if (line == "isready") {
            std::cout << "readyok" << endl;
            std::cout.flush();
        }
        else if (line == "ucinewgame") {
            board.setFen(constants::STARTPOS);
            clearHashTable();
        }
        else if (line.substr(0, 8) == "position") {
            vector<string> tokens = splitString(line, ' ');
            if (tokens.size() < 2) continue;
            
            try {
                if (tokens[1] == "startpos") {
                    board.setFen(constants::STARTPOS);
                    bool foundMoves = false;
                    for (size_t i = 2; i < tokens.size(); ++i) {
                        if (tokens[i] == "moves") {
                            foundMoves = true;
                            for (size_t j = i + 1; j < tokens.size(); ++j) {
                                try {
                                    Move mv = uci::uciToMove(board, tokens[j]);
                                    board.makeMove(mv);
                                } catch (...) {
                                    break;
                                }
                            }
                            break;
                        }
                    }
                }
                else if (tokens[1] == "fen" && tokens.size() >= 8) {
                    string fen = tokens[2];
                    for (size_t i = 3; i < 8 && i < tokens.size(); ++i) {
                        fen += " " + tokens[i];
                    }
                    board.setFen(fen);
                
                    for (size_t i = 8; i < tokens.size(); ++i) {
                        if (tokens[i] == "moves") {
                            for (size_t j = i + 1; j < tokens.size(); ++j) {
                                try {
                                    Move mv = uci::uciToMove(board, tokens[j]);
                                    board.makeMove(mv);
                                } catch (...) {
                                    break;
                                }
                            }
                            break;
                        }
                    }
                }
            } catch (...) {
                board.setFen(constants::STARTPOS);
            }
        }
        else if (line.substr(0, 2) == "go") {
            vector<string> tokens = splitString(line, ' ');
            int depth = 7;

            for (size_t i = 1; i + 1 < tokens.size(); ++i) {
                if (tokens[i] == "depth") {
                    try {
                        depth = stoi(tokens[i + 1]);
                        depth = max(1, min(depth, 15));
                    } catch (...) {
                        depth = 7;
                    }
                    break;
                }
            }
            
            Movelist rootMoves;
            movegen::legalmoves<>(rootMoves, board);
            
            if (rootMoves.empty()) {
                std::cout << "bestmove 0000" << endl;
                std::cout.flush();
                continue;
            }
            
            if (rootMoves.size() == 1) {
                std::cout << "bestmove " << uci::moveToUci(rootMoves[0]) << endl;
                std::cout.flush();
                continue;
            }
            
            Move bestMove = iterativedeep(board, depth);
            
            if (bestMove == Move::NULL_MOVE) {
                bestMove = rootMoves[0];
            }
            
            std::cout << "bestmove " << uci::moveToUci(bestMove) << endl;
            std::cout.flush();
        }
        else if (line == "quit") {
            break;
        }
    }
    return 0;
}