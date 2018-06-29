#include "Pattern.h"

using namespace std;
using namespace Gomoku;
using Eigen::Matrix;

constexpr Direction Directions[] = {
    Direction::Vertical, Direction::Horizontal, Direction::LeftDiag, Direction::RightDiag
};

template <size_t Length = TARGET_LEN, typename Array_t>
inline auto& LineView(Array_t& src, Position move, Direction dir) {
    static vector<Array_t::pointer> view_ptrs(Length);
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

template <int Size = MAX_SURROUNDING_SIZE, typename Array_t>
inline auto BlockView(Array_t& src, Position move) {
    static Eigen::Map<Matrix<Array_t::value_type, HEIGHT, WIDTH>> view(nullptr);
    new (&view) decltype(view)(src.data()); // placement new
    auto left_bound  = std::max(move.x() - Size / 2, 0);
    auto right_bound = std::min(move.x() + Size / 2, WIDTH - 1);
    auto up_bound    = std::max(move.y() - Size / 2, 0);
    auto down_bound  = std::min(move.y() + Size / 2, HEIGHT - 1);
    Position lu{ left_bound, up_bound }, rd{ right_bound, down_bound };
    Position origin = move - Position{ Size / 2, Size / 2 };
    return make_tuple(
        view.block(lu.x(), lu.y(), rd.x() - lu.x() + 1, rd.y() - lu.y() + 1),
        Position(lu - origin), Position(rd - origin)
    );
}

inline const auto SURROUNDING_WEIGHTS = []() {
    Eigen::Array<
        int, MAX_SURROUNDING_SIZE, MAX_SURROUNDING_SIZE
    > W;
    W << 2, 0, 0, 2, 0, 0, 2,
         0, 4, 3, 3, 3, 4, 0,
         0, 3, 5, 4, 5, 3, 0,
         1, 3, 4, 0, 4, 3, 1,
         0, 3, 5, 4, 5, 3, 0,
         0, 4, 3, 3, 3, 4, 0,
         2, 0, 0, 2, 0, 0, 2;
    return W;
}();

/* ------------------- Pattern类实现 ------------------- */

Pattern::Pattern(std::string_view proto, Type type, int score) 
    : str(proto.begin() + 1, proto.end()), score(score), type(type),
      belonging(proto[0] == '+' ? Player::Black : Player::White) {
    
}

/* ------------------- PatternSearch类实现 ------------------- */

constexpr int TrieCharset(char ch) {
    switch (int code = 0; ch) {
        case '-': case '_': case '^': case '~': ++code;
        case '?': ++code;
        case 'o': ++code;
        case 'x': ++code;
        default: return code;
    }
}

// 用于生成完整的Pattern集合的一次性工具类。
class FullPatterns {
public:
    FullPatterns(initializer_list<Pattern> protos) : m_patterns(protos) {
        this->reverseAugment()->flipAugment()->boundaryAugment();
    }

    std::vector<Pattern> release() { return std::move(m_patterns); }

private:
    FullPatterns* reverseAugment() {
        for (int i = 0, size = m_patterns.size(); i < size; ++i) {
            Pattern reversed(m_patterns[i]);
            std::reverse(reversed.str.begin(), reversed.str.end());
            if (reversed.str != m_patterns[i].str) {
                m_patterns.push_back(std::move(reversed));
            }
        }
        return this;
    }

    FullPatterns* flipAugment() {
        for (int i = 0, size = m_patterns.size(); i < size; ++i) {
            Pattern flipped(m_patterns[i]);
            flipped.belonging = -flipped.belonging;
            for (auto& piece : flipped.str) {
                if (piece == 'x') piece = 'o';
                else if (piece == 'o') piece = 'x';
            }
            m_patterns.push_back(flipped);
        }
        return this;
    }

    FullPatterns* boundaryAugment() {
        for (int i = 0, size = m_patterns.size(); i < size; ++i) {
            char enemy = m_patterns[i].belonging == Player::Black ? 'x' : 'o';
            auto first = m_patterns[i].str.find_first_of(enemy);
            auto last  = m_patterns[i].str.find_last_of(enemy);
            if (first != string::npos) { // 最多只有三种情况
                Pattern bounded(m_patterns[i]);
                bounded.str[first] = '?';
                m_patterns.push_back(bounded);
                if (last != first) {
                    bounded.str[last] = '?';
                    m_patterns.push_back(bounded);
                    bounded.str[first] = enemy;
                    m_patterns.push_back(std::move(bounded));
                }
            }
        }
        return this;
    }

    vector<Pattern> m_patterns;
};

inline void BuildTrie(PatternSearch* searcher, FullPatterns&& fp) {
    // 用于构建双数组的临时结点
    struct Node {

    };
    // pattern转换为int编码
    // 按照int编码字典序排序
    // 插入生成Trie树
    
}

inline void BuildAhoCorasickGraph(PatternSearch* searcher) {

}

PatternSearch::PatternSearch(initializer_list<Pattern> protos) {
    BuildTrie(this, FullPatterns(protos));
    BuildAhoCorasickGraph(this);
}

/* ------------------- BoardMap类实现 ------------------- */

BoardMap::BoardMap(Board* board) : m_board(board ? board : new Board) {
    this->reset();
}

string_view BoardMap::lineView(Position pose, Direction direction) {
    const auto [index, offset] = parseIndex(pose, direction);
    return string_view(&m_lineMap[index][offset - TARGET_LEN / 2],  TARGET_LEN);
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
    m_hash = 0ul;
    m_board->reset();
    for (auto& line : m_lineMap) {
        line.resize(MAX_PATTERN_LEN - 1, '?'); // 线前填充越界位('?')
    }
    for (auto i = 0; i < BOARD_SIZE; ++i) 
    for (auto direction : Directions) {
        auto [index, _] = parseIndex(i, direction);
        m_lineMap[index].push_back('-'); // 按每个位置填充空位('-')
    }
    for (auto& line : m_lineMap) {
        line.append(MAX_PATTERN_LEN - 1, '?'); // 线后填充越界位('?')
    }
}

constexpr tuple<int, int> BoardMap::parseIndex(Position pose, Direction direction) {
    // 由于前与后MAX_PATTERN_LEN - 1位均为'?'（越界位），故需设初始offset
    switch (int offset = MAX_PATTERN_LEN - 1; direction) {      
        case Direction::Horizontal: // 0 + x∈[0, WIDTH) | y: 0 -> HEIGHT
            return { pose.x(), offset + pose.y() };
        case Direction::Vertical:   // WIDTH + y∈[0, HEIGHT) | x: 0 -> WIDTH
            return { WIDTH + pose.y(), offset + pose.x() };
        case Direction::LeftDiag:   // (WIDTH + HEIGHT) + (HEIGHT - 1) + x-y∈[-(HEIGHT - 1), WIDTH) | min(x, y): (x, y) -> (x+1, y+1)
            return { WIDTH + 2*HEIGHT - 1 + pose.x() - pose.y(), offset + std::min(pose.x(), pose.y()) };
        case Direction::RightDiag:  // 2*(WIDTH + HEIGHT) - 1 + x+y∈[0, WIDTH + HEIGHT - 1) | min(WIDTH - x, y): (x, y) -> (x-1, y+1)
            return { 2*(WIDTH + HEIGHT) - 1 + pose.x() + pose.y(), offset + std::min(WIDTH - pose.x(), pose.y()) };
        default:
            throw direction; 
    }
}

/* ------------------- Evaluator类实现 ------------------- */

Evaluator::Evaluator(Board * board) : m_boardMap(board) {
    this->reset();
}

// 如果是己方的棋型，则必须下在有效空位('_'型空位）
// 否则，下在其他空位也可能封住该棋型('_' + '^'型空位）（'^'不一定有作用，但会在1~2次迭代内被软剪枝掉）
int Evaluator::patternCount(Position move, Pattern::Type type, Player perspect, Player curPlayer) {
    auto raw_counts = distribution(perspect, type)[move];
    auto critical_blanks = raw_counts & ((1 << 8 * sizeof(int) / 2) - 1); // '_'型空位
    auto irrelevant_blanks = raw_counts >> (8 * sizeof(int) / 2); // '^'型空位
    return perspect == curPlayer ? critical_blanks : critical_blanks + irrelevant_blanks;
}

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

void Evaluator::updateOnePiece(int delta, Position move, Player src_player) {
    auto sgn = [](int x) { return x < 0 ? -1 : 1; }; // 0以上记为正（认为原位置没有棋）
    auto [view, lu, rd] = BlockView(distribution(src_player, Pattern::One), move);
    auto weight_block = SURROUNDING_WEIGHTS.block(lu.x(), lu.y(), rd.x() - lu.x() + 1, rd.y() - lu.y() + 1);
    view.array() += view.array().unaryExpr(sgn) * delta * weight_block; // sgn对应原位置是否有棋，delta对应下棋还是悔棋
    for (auto perspect : { Player::Black, Player::White }) {
        auto& count = distribution(perspect, Pattern::One)[move];
        switch (delta) {
        case 1: // 代表这里下了一子
            count *= -1; // 反转计数，表明这个位置已有棋子占用
            count -= 1; // 保证count一定是负数
            break;
        case -1: // 代表悔了这一子
            count += 1; // 除去之前减掉的辅助数
            count *= -1; // 反转计数，表明这里可以用来计分了
            break;
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

void Evaluator::reset() {
    m_boardMap.reset();
    for (auto& temp : m_distributions)
    for (auto& distribution : temp) {
        distribution.resize(BOARD_SIZE, 0);
    }
}

// 以下模式被设计为没有互为前缀码的情况。
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
    { "-x_o___x", Pattern::DeadOne,   35 },
    { "-x__o__x", Pattern::DeadOne,   40 },
};
