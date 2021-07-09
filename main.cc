// Game of Othello -- Example of main
// Universidad Simon Bolivar, 2012.
// Author: Blai Bonet
// Last Revision: 1/11/16
// Modified by:

#include <iostream>
#include <limits>
#include "othello_cut.h" // won't work correctly until .h is fixed!
#include "utils.h"

#include <unordered_map>

using namespace std;

unsigned expanded = 0;
unsigned generated = 0;
int tt_threshold = 32; // threshold to save entries in TT
const int INF = 200;

// Transposition table (it is not necessary to implement TT)
struct stored_info_t {
    int value_;
    int type_;
    enum { EXACT, LOWER, UPPER };
    stored_info_t(int value = -100, int type = LOWER) : value_(value), type_(type) { }
};

struct hash_function_t {
    size_t operator()(const state_t &state) const {
        return state.hash();
    }
};

class hash_table_t : public unordered_map<state_t, stored_info_t, hash_function_t> {
};

hash_table_t TTable[2];

//int maxmin(state_t state, int depth, bool use_tt);
//int minmax(state_t state, int depth, bool use_tt = false);
//int maxmin(state_t state, int depth, bool use_tt = false);
int negamax(state_t state, int color);
int negamax(state_t state, int alpha, int beta, int color);
int scout(state_t state, int color);
int negascout(state_t state, int alpha, int beta, int color);

int main(int argc, const char **argv) {
    state_t pv[128];
    int npv = 0;
    for ( int i = 0; PV[i] != -1; ++i ) ++npv;

    int algorithm = 0;
    if ( argc > 1 ) algorithm = atoi(argv[1]);
    bool use_tt = argc > 2;

    // Extract principal variation of the game
    state_t state;
    cout << "Extracting principal variation (PV) with " << npv << " plays ... " << flush;
    for ( int i = 0; PV[i] != -1; ++i ) {
        bool player = i % 2 == 0; // black moves first!
        int pos = PV[i];
        pv[npv - i] = state;
        state = state.move(player, pos);
    }
    pv[0] = state;
    cout << "done!" << endl;

#if 0
    // print principal variation
    for ( int i = 0; i <= npv; ++i )
        cout << pv[npv - i];
#endif

    // Print name of algorithm
    cout << "Algorithm: ";
    if ( algorithm == 1 )
        cout << "Negamax (minmax version)";
    else if ( algorithm == 2 )
        cout << "Negamax (alpha-beta version)";
    else if ( algorithm == 3 )
        cout << "Scout";
    else if ( algorithm == 4 )
        cout << "Negascout";
    cout << (use_tt ? " w/ transposition table" : "") << endl;

    // Run algorithm along PV (bacwards)
    cout << "Moving along PV:" << endl;
    for ( int i = 0; i <= npv; ++i ) {
        //cout << pv[i];
        int value = 0;
        TTable[0].clear();
        TTable[1].clear();
        float start_time = Utils::read_time_in_seconds();
        expanded = 0;
        generated = 0;
        int color = i % 2 == 1 ? 1 : -1;

        try {
            if ( algorithm == 1 ) {
                value = negamax(pv[i], color);
            } else if ( algorithm == 2 ) {
                value = negamax(pv[i], -INF, INF, color);
            } else if ( algorithm == 3 ) {
                value = scout(pv[i], color);
            } else if ( algorithm == 4 ) {
                value = negascout(pv[i], -INF, INF, color);
            }
        } catch ( const bad_alloc &e ) {
            cout << "size TT[0]: size=" << TTable[0].size() << ", #buckets=" << TTable[0].bucket_count() << endl;
            cout << "size TT[1]: size=" << TTable[1].size() << ", #buckets=" << TTable[1].bucket_count() << endl;
            use_tt = false;
        }

        float elapsed_time = Utils::read_time_in_seconds() - start_time;

        cout << npv + 1 - i << ". " << (color == 1 ? "Black" : "White") << " moves: "
             << "value=" << (algorithm != 3 ? color : 1) * value
             << ", #expanded=" << expanded
             << ", #generated=" << generated
             << ", seconds=" << elapsed_time
             << ", #generated/second=" << generated / elapsed_time
             << endl;
    }

    return 0;
}


int negamax(state_t state, int color) {
    ++generated;
    if (state.terminal())
        return color * state.value();

    int score = -INF;
    // booleano para saber si logre moverme colocando alguna ficha
    // o si tengo que pasar el turno al otro
    bool moved = false;
    for (int p : state.get_moves(color == 1)) {
        moved = true;
        score = max(
                    score,
                    -negamax(state.move(color == 1, p), -color)
                );
    }
    // si no logre moverme sigo en el mismo estado pero cambio el color
    if (!moved)
        score = -negamax(state, -color);

    ++expanded;
    return score;
}

int negamax(state_t state, int alpha, int beta, int color) {
    ++generated;
    if (state.terminal())
        return color * state.value();

    int score = -INF;
    // booleano para saber si logre moverme colocando alguna ficha
    // o si tengo que pasar el turno al otro
    bool moved = false;
    for (int p : state.get_moves(color == 1)) {
        moved = true;
        score = max(
                    score,
                    -negamax(state.move(color == 1, p), -beta, -alpha, -color)
                );
        alpha = max(alpha, score);
        if (alpha >= beta)
            break;
    }
    // si no logre moverme sigo en el mismo estado pero cambio el color
    if (!moved)
        score = -negamax(state, -beta, -alpha, -color);

    ++expanded;
    return score;
}

// cond : 0 es > ; 1 es >=
bool test(state_t state, int color, int score, bool cond) {

    ++generated;
    if (state.terminal())
        return (cond ? state.value() >= score : state.value() > score);

    ++expanded;
    auto moves = state.get_moves(color == 1);
    for (int i = 0; i < (int)moves.size(); ++i) {
        auto child = state.move(color == 1, moves[i]);
        if (color == 1 && test(child, -color, score, cond))
            return true;
        if (color == -1 && !test(child, -color, score, cond))
            return false;
    }

    if (moves.size() == 0) {
        if (color == 1 && test(state, -color, score, cond))
            return true;
        if (color == -1 && !test(state, -color, score, cond))
            return false;
    }

    return color == -1;
}

int scout(state_t state, int color) {
    ++generated;
    if (state.terminal())
        return state.value();

    int score = 0;
    auto moves = state.get_moves(color == 1);
    for (int i = 0; i < (int)moves.size(); ++i) {
        auto child = state.move(color == 1, moves[i]);
        // primer hijo
        if (i == 0)
            score = scout(child, -color);
        else {
            if (color == 1 && test(child, -color, score, 0))
                score = scout(child, -color);
            if (color == -1 && !test(child, -color, score, 1))
                score = scout(child, -color);
        }
    }
    // no se logro poner fichas, pasar turno
    if (moves.size() == 0)
        score = scout(state, -color);

    ++expanded;
    return score;
}

int negascout(state_t state, int alpha, int beta, int color) {

    ++generated;
    if (state.terminal())
        return color * state.value();

    int score;
    auto moves = state.get_moves(color == 1);
    for (int i = 0; i < (int)moves.size(); ++i) {
        auto child = state.move(color == 1, moves[i]);
        // primer hijo
        if (i == 0)
            score = -negascout(child, -beta, -alpha, -color);
        else {

            score = -negascout(child, -alpha - 1, -alpha, -color);
            if (alpha < score && score < beta)
                score = -negascout(child, -beta, -score, -color);
        }

        alpha = max(alpha, score);
        if (alpha >= beta)
            break;
    }

    // no se logro poner fichas, pasar turno
    if (moves.size() == 0)
        alpha = -negascout(state, -beta, -alpha, -color);


    ++expanded;
    return alpha;
}
