#include "Pattern.h"

using namespace std;
using namespace Gomoku;

constexpr Direction Directions[] = {
    Direction::Vertical, Direction::Horizontal, Direction::LeftDiag, Direction::RightDiag
};

template <size_t Length = SAMPLE_LEN, typename View_t>
inline auto& LineView(array<View_t, BOARD_SIZE>& src, Position move, Direction dir) {
    static vector<View_t*> view_ptrs(Length);
    auto [dx, dy] = *dir;
    for (int i = 0, j = i - Length / 2; i < Length; ++i, ++j) {
        auto x = move.x() + dx * j, y = move.y() + dy * j;
        if ((x >= 0 && x < WIDTH) && (y >= 0 && y < HEIGHT)) {
            view_ptrs[i] = &src[Position{ x, y }];
        }
        else {
            view_ptrs[i] = nullptr;
        }
    }
    return view_ptrs;
}

template <int Size = MAX_SURROUNDING_SIZE, typename View_t>
inline auto BlockView(array<View_t, BOARD_SIZE>& src, Position move) {
    static Eigen::Map<Eigen::Matrix<View_t, HEIGHT, WIDTH>> view(nullptr);
    new (&view) decltype(view)(src.data()); // placement new
    auto left_bound  = std::max(move.x() - Size / 2, 0);
    auto right_bound = std::min(move.x() + Size / 2, WIDTH - 1);
    auto up_bound    = std::max(move.y() - Size / 2, 0);
    auto down_bound  = std::min(move.y() + Size / 2, HEIGHT - 1);
    Position lr{ left_bound, up_bound }, rd{ right_bound, down_bound };
    Position origin = move - Position{ Size / 2, Size / 2 };
    return make_tuple(
        view.block(lr.x(), lr.y(), rd.x() - lr.x() + 1, rd.y() - lr.y() + 1),
        Position(lr - origin), Position(rd - origin)
    );
}

/* ------------------- Pattern类实现 ------------------- */

Pattern::Pattern(std::string_view proto, Type type, int score) 
    : str(proto.begin() + 1, proto.end()), score(score), type(type),
      belonging(proto[0] == '+' ? Player::Black : Player::White) {
    
}

/* ------------------- PatternSearch类实现 ------------------- */

PatternSearch::PatternSearch(std::initializer_list<Pattern> protos) {
    for (auto&& pattern : protos) {
        // TODO
    }
}

/* ------------------- BoardMap类实现 ------------------- */

BoardMap::BoardMap(Board* board) : m_board(board ? board : new Board) {
    this->reset();
}

string_view BoardMap::lineView(Position pose, Direction direction) {
    const auto [index, offset] = parseIndex(pose, direction);
    return string_view(&m_lineMap[index][offset - SAMPLE_LEN / 2],  SAMPLE_LEN);
}

Player BoardMap::applyMove(Position move) {
    for (auto direction : Directions) {
        const auto [index, offset] = parseIndex(move, direction);
        m_lineMap[index][offset] = m_board->m_curPlayer == Player::Black ? 'x' : 'o';
    }
    return m_board->applyMove(move, false);
}

Player BoardMap::revertMove(size_t count) {
    for (int i = 0; i < count; ++i) {
        for (auto direction : Directions) {
            const auto [index, offset] = parseIndex(m_board->m_moveRecord.back(), direction);
            m_lineMap[index][offset] = '-';
        }
        m_board->revertMove();
    }
    return m_board->m_curPlayer;
}

void BoardMap::reset() {
    
}

tuple<int, int> BoardMap::parseIndex(Position pose, Direction direction) {
    switch (direction) {
    case Direction::Horizontal: // 0 + x∈[0, WIDTH), y
        return { pose.x(), pose.y() };
    case Direction::Vertical:   // WIDTH + y∈[0, HEIGHT), x
        return { WIDTH + pose.y(), pose.x() };
    case Direction::LeftDiag:   // (WIDTH + HEIGHT) + (HEIGHT - 1) + x-y∈[-(HEIGHT - 1), WIDTH) // TODO: implement here
        return { WIDTH + 2*HEIGHT - 1 + pose.x() - pose.y(), 1 };
    case Direction::RightDiag:  // 2*(WIDTH + HEIGHT) - 1 + x+y∈[0, WIDTH + HEIGHT - 1)
        return { 2*(WIDTH + HEIGHT) - 1 + pose.x() + pose.y(), 1 };
    default:
        throw direction; 
    }
}

/* ------------------- Evaluator类实现 ------------------- */

void Evaluator::updatePattern(string_view target, int delta, Position move, Direction dir) {
    for (auto [pattern, offset] : Patterns.matches(target)) {
        if (pattern.type == Pattern::Five) {
            // 此前board已完成applyMove，故此处设置curPlayer为None不会发生阻塞。
            board().m_curPlayer = Player::None;
            board().m_winner = pattern.belonging;
        } else {
            auto view = LineView(distribution(pattern.belonging, pattern.type), move, dir);
            for (auto idx = 0; idx < pattern.str.length(); ++idx) {
                auto offset = 0;
                switch (pattern.str[idx]) {
                case '-': offset = 0; break;
                case '^': offset = 8 * sizeof(int) / 2; break;
                default: continue;
                }
                auto view_idx = offset + idx;
                *view[view_idx] += delta * (1 << offset);
            }
        }
    }
}

void Evaluator::updateMove(Position move, Player src_player) {
    for (auto dir : Directions) {
        auto target = m_boardMap.lineView(move, dir);
        updatePattern(target, -1, move, dir); // remove pattern count on original state
    }
    if (src_player != Player::None) {
        m_boardMap.applyMove(move);
        updateOnePiece(1, move, board().m_curPlayer);
    } else {
        m_boardMap.revertMove();
        updateOnePiece(-1, move, -board().m_curPlayer | board().m_winner);
    }
    for (auto dir : Directions) {
        auto target = m_boardMap.lineView(move, dir);
        updatePattern(target, 1, move, dir); // update pattern count on new state
    }
}

Player Evaluator::applyMove(Position move) {
    if (board().m_curPlayer != Player::None && board().checkMove(move)) {
        updateMove(move, board().m_curPlayer);
    }
    return board().m_curPlayer;
}

Player Evaluator::revertMove(size_t count) {
    for (auto i = 0; i < count && !board().m_moveRecord.empty(); ++i) {
        updateMove(board().m_moveRecord.back(), Player::None);
    }
    return board().m_curPlayer;
}

bool Evaluator::checkGameEnd() {
    if (board().m_curPlayer == Player::None) {
        return true;
    }

    if (board().moveCounts(Player::None) == 0) {
        board().m_winner = Player::None;
        board().m_curPlayer = Player::None;
        return true;
    }

    return false;
}

PatternSearch Evaluator::Patterns = {
    { "+xxxxx",   Pattern::Five,      99999 },
    { "-_oooo_",  Pattern::LiveFour,  75000 },
    { "-xoooo_",  Pattern::DeadFour,  2500 },
    { "-o_ooo",   Pattern::DeadFour,  3000 },
    { "-oo_oo",   Pattern::DeadFour,  2600 },
    { "-~_ooo^",  Pattern::LiveThree, 3000 },
    { "-^o_oo^",  Pattern::LiveThree, 2800 },
    { "-xooo__",  Pattern::DeadThree, 500 },
    { "-xoo_o_",  Pattern::DeadThree, 800 },
    { "-xo_oo_",  Pattern::DeadThree, 999 },
    { "-oo__o",   Pattern::DeadThree, 600 },
    { "-o_o_o",   Pattern::DeadThree, 550 },
    { "-x_ooo_x", Pattern::DeadThree, 400 },
    { "-^oo__~",  Pattern::LiveTwo,   650 },
    { "-^_o_o_^", Pattern::LiveTwo,   600 },
    { "-x^o_o_^", Pattern::LiveTwo,   550 },
    { "-^o__o^",  Pattern::LiveTwo,   550 },
    { "-xoo___",  Pattern::DeadTwo,   150 },
    { "-xo_o__",  Pattern::DeadTwo,   250 },
    { "-xo__o_",  Pattern::DeadTwo,   200 },
    { "-o___o",   Pattern::DeadTwo,   180 },
    { "-x_oo__x", Pattern::DeadTwo,   100 },
    { "-x_o_o_x", Pattern::DeadTwo,   100 },
    { "-^o___~",  Pattern::LiveOne,   150 },
    { "-x^_o__^", Pattern::LiveOne,   140 },
    { "-x^__o_^", Pattern::LiveOne,   150 },
    { "-xo___~",  Pattern::DeadOne,   30 },
    { "-x_o___x", Pattern::DeadOne,   50 },
    { "-x__o__x", Pattern::DeadOne,   60 },
};
