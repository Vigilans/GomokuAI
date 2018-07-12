#include "Pattern.h"
#include "ACAutomata.h"
#include <iostream>

using namespace std;
using namespace Eigen;

namespace Gomoku {

/* ------------------- Pattern类实现 ------------------- */

Pattern::Pattern(std::string_view proto, Type type, int score) 
    : str(proto.begin() + 1, proto.end()), score(score), type(type),
      favour(proto[0] == '+' ? Player::Black : Player::White) {

}

/* ------------------- PatternSearch类实现 ------------------- */

PatternSearch::PatternSearch(initializer_list<Pattern> protos) {
    AhoCorasickBuilder builder(protos);
    builder.build(this);
}

// 持续转移，直到匹配下一个成功的模式或查询结束
const PatternSearch::generator& PatternSearch::generator::operator++() {
    do {
        if (target.empty()) { // 目标匹配完全结束
            state = 0; // 将状态重置为初始状态
            break; // 此次break后，该generator达到了end状态
        }
        int code = target[0]; // 取出当前要检测的目标位
        if (code == EncodeCharset('?') && state == code) { // 相当于判断state == base[0] + code('?')
            while (!target.empty() && target[0] == code) { // 快速跳转冗长的'?'状态
                ++offset, target.remove_prefix(1); 
            }
            continue;
        }
        int next = ref->m_base[state] + code;
        if (ref->m_check[next] == state) { // 尝试往下匹配target
            state = next; // 匹配成功转移
        } else if (state != 0) {
            state = ref->m_fail[state]; // 匹配失败转移
            continue; // 失败转移后在下一循环再次尝试匹配
        } else {
            target = ""; // 清空目标串，认为匹配已结束
            break; // 若在根节点匹配失败，则没有再搜索的意义
        }
        ++offset, target.remove_prefix(1);
    } while (ref->m_check[ref->m_base[state]] != state); // 发现叶子结点时，暂时中断匹配
    return *this;
}

// 解析当前成功匹配到的模式
PatternSearch::Entry PatternSearch::generator::operator*() const {
    auto leaf = ref->m_base[state];
    return { ref->m_patterns[-ref->m_base[leaf]], offset };
}

PatternSearch::generator PatternSearch::execute(string_view target) {
    return generator{ target, this };
}

const vector<PatternSearch::Entry>& PatternSearch::matches(string_view target) {
    static vector<Entry> entries;
    entries.clear();
    for (auto record : execute(target)) { 
        entries.push_back(record); 
    }
    return entries;
}

/* ------------------- BoardMap类实现 ------------------- */

tuple<int, int> BoardMap::ParseIndex(Position pose, Direction direction) {
    // 由于前与后MAX_PATTERN_LEN - 1位均为'?'（越界位），故需设初始offset
    switch (int offset = MAX_PATTERN_LEN - 1; direction) {
        case Direction::Horizontal: // 0 + y∈[0, HEIGHT) | x: 0 -> WIDTH
            return { pose.y(), offset + pose.x() };
        case Direction::Vertical:   // HEIGHT + x∈[0, WIDTH) | y: 0 -> HEIGHT
            return { HEIGHT + pose.x(), offset + pose.y() };
        case Direction::LeftDiag:   // (WIDTH + HEIGHT) + (HEIGHT - 1) + x-y∈[-(HEIGHT - 1), WIDTH) | min(x, y): (x, y) -> (x+1, y+1)
            return { WIDTH + 2 * HEIGHT - 1 + pose.x() - pose.y(), offset + std::min(pose.x(), pose.y()) };
        case Direction::RightDiag:  // 2*(WIDTH + HEIGHT) - 1 + x+y∈[0, WIDTH + HEIGHT - 1) | min(WIDTH - 1 - x, y): (x, y) -> (x-1, y+1)
            return { 2 * (WIDTH + HEIGHT) - 1 + pose.x() + pose.y(), offset + std::min(WIDTH - 1 - pose.x(), pose.y()) };
        default:
            throw direction;
    }
}

BoardMap::BoardMap(Board* board) : m_board(board ? board : new Board) {
    this->reset();
}

string_view BoardMap::lineView(Position pose, Direction direction) {
    const auto [index, offset] = ParseIndex(pose, direction);
    return string_view(&m_lineMap[index][offset - TARGET_LEN / 2],  TARGET_LEN);
}

Player BoardMap::applyMove(Position move) {
    for (auto direction : Directions) {
        const auto [index, offset] = ParseIndex(move, direction);
        m_lineMap[index][offset] = EncodeCharset(m_board->m_curPlayer == Player::Black ? 'x' : 'o');
    }
    return m_board->applyMove(move, false);
}

Player BoardMap::revertMove(size_t count) {
    for (int i = 0; i < count; ++i) {
        for (auto direction : Directions) {
            const auto [index, offset] = ParseIndex(m_board->m_moveRecord.back(), direction);
            m_lineMap[index][offset] = EncodeCharset('-');
        }
        m_board->revertMove();
    }
    return m_board->m_curPlayer;
}

void BoardMap::reset() {
    m_hash = 0ul;
    m_board->reset();
    for (auto& line : m_lineMap) {
        line.resize(MAX_PATTERN_LEN - 1, EncodeCharset('?')); // 线前填充越界位('?')
    }
    for (auto i = 0; i < BOARD_SIZE; ++i) 
    for (auto direction : Directions) {
        auto [index, _] = ParseIndex(i, direction);
        m_lineMap[index].push_back(EncodeCharset('-')); // 按每个位置填充空位('-')
    }
    for (auto& line : m_lineMap) {
        line.append(MAX_PATTERN_LEN - 1, EncodeCharset('?')); // 线后填充越界位('?')
    }
}

/* ------------------- Evaluator类实现 ------------------- */

template <size_t Length = TARGET_LEN, typename Array_t>
inline auto& LineView(Array_t& src, Position move, Direction dir) {
    static vector<typename Array_t::value_type*> view_ptrs(Length);
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

template <int Size = BLOCK_SIZE, typename Array_t, typename value_t = Array_t::value_type>
inline auto BlockView(Array_t& src, Position move) {
    auto left_bound  = std::max(move.x() - Size / 2, 0);
    auto right_bound = std::min(move.x() + Size / 2, WIDTH - 1);
    auto up_bound    = std::max(move.y() - Size / 2, 0);
    auto down_bound  = std::min(move.y() + Size / 2, HEIGHT - 1);
    Position lu{ left_bound, up_bound }, rd{ right_bound, down_bound };
    if constexpr (std::is_same_v<Array_t, Array<value_t, Size, Size, RowMajor>>) { // 权重矩阵落在这里
        Position coord_transform = move - Position{ Size / 2, Size / 2 };
        lu = lu - coord_transform, rd = rd - coord_transform;
        return src.block(lu.y(), lu.x(), rd.y() - lu.y() + 1, rd.x() - lu.x() + 1);
    } else { // 一般board-like行优先连续数组落在这里
        Eigen::Map<Array<value_t, HEIGHT, WIDTH, RowMajor>> view(src.data());
        return view.block(lu.y(), lu.x(), rd.y() - lu.y() + 1, rd.x() - lu.x() + 1);
    }
}

void Evaluator::updateLine(string_view target, int delta, Position move, Direction dir) {
    for (auto [pattern, offset] : Patterns.execute(target)) {
        if (pattern.type == Pattern::Five) {
            // 此前board已完成applyMove，故此处设置curPlayer为None不会发生阻塞。
            board().m_curPlayer = Player::None;
            board().m_winner = pattern.favour;
            continue;
        }
        auto update_pattern = [&]() {
            m_patternDist.back()[pattern.type].set(delta, pattern.favour); // 修改总计数
            for (int i = 0; i < pattern.str.length(); ++i) { // 修改空位数据
                auto piece = pattern.str.rbegin()[i];
                if (piece == '_' || piece == '^') { // '_'代表己方有效空位，'^'代表对方反制空位
                    auto pose = Shift(move, offset - i - TARGET_LEN / 2, dir);
                    auto multiplier = (dir == Direction::LeftDiag || dir == Direction::RightDiag ? 1.2 : 1);
                    auto score = int(delta * multiplier * pattern.score);
                    switch (piece) { // 利用了switch的穿透特性
                    case '_':
                        m_patternDist[pose][pattern.type].set(delta, pattern.favour, pattern.favour, dir);
                        scores(pattern.favour, pattern.favour)[pose] += score;
                        assert(scores(pattern.favour, pattern.favour)[pose] >= 0);
                    case '^':
                        m_patternDist[pose][pattern.type].set(delta, pattern.favour, -pattern.favour, dir);
                        scores(pattern.favour, -pattern.favour)[pose] += score;
                        assert(scores(pattern.favour, -pattern.favour)[pose] >= 0);
                    }
                }
            }
        };
        auto update_compound = [&]() {
            for (int i = 0; i < pattern.str.length(); ++i) {
                auto piece = pattern.str.rbegin()[i];
                auto pose = Shift(move, offset - i - TARGET_LEN / 2, dir);
                if ((piece == '_' || piece == '^') && 
                    Compound::Test(*this, pose, pattern.favour)) {
                    Compound::Update(*this, delta, pose, pattern.favour);
                }
            }
        };
        if (delta == 1) {
            update_pattern(), update_compound();
        } else {
            update_compound(), update_pattern();
        }
    }
}

void Evaluator::updateBlock(int delta, Position move, Player src_player) {
    // 数据准备
    auto sgn = [](int x) { return x < 0 ? -1 : 1; }; // 0以上记为正（认为原位置没有棋）
    auto relu = [](int x) { return std::max(x, 0); };
    auto [weights, score] = BlockWeights;
    auto density_block = BlockView(m_density[Group(src_player)], move);
    auto score_block   = BlockView(m_scores[Group(src_player)], move);
    auto weight_block  = BlockView(weights, move);
    // 除去旧density分数（初始情况下density_block为0矩阵，减去也没有影响）
    //score_block -= score * density_block.unaryExpr(relu);    
    // 更新density（sgn对应原位置是否有棋，delta对应下棋还是悔棋）。
    density_block += density_block.unaryExpr(sgn) * delta * weight_block;
    // 修改中心位置的状态（是否可以用来计分）
    for (auto perspective : { Player::Black, Player::White }) {
        switch (auto& count = m_density[Group(perspective)][move]; delta) {
        case 1: // 代表这里下了一子
            count *= -1, count -= 1; break; // 反转计数，表明这个位置已有棋子占用，并减去一辅助数，保证count一定是负数
        case -1: // 代表悔了这一子
            count += 1, count *= -1; break; // 除去之前减掉的辅助数后，反转计数，表明这里可以用来计分了
        }
    }
    //cout << "Current: " << to_string(src_player) << endl;
    //cout << endl << "White:\n" << Map<Array<int, WIDTH, HEIGHT, RowMajor>>(m_density[0].data()) << endl
    //     << "Black:\n" << Map<Array<int, WIDTH, HEIGHT, RowMajor>>(m_density[1].data()) << endl;
    // 补回新density分数
    //score_block += score * density_block.unaryExpr(relu);
}

void Evaluator::updateMove(Position move, Player src_player) {
    for (auto dir : Directions) {
        auto target = m_boardMap.lineView(move, dir);
        updateLine(target, -1, move, dir); // remove pattern count on original state
    }
    if (src_player != Player::None) {
        m_boardMap.applyMove(move);
        updateBlock(1, move, src_player);
    } else {
        m_boardMap.revertMove();
        updateBlock(-1, move, board().m_curPlayer);
    }
    for (auto dir : Directions) {
        auto target = m_boardMap.lineView(move, dir);
        updateLine(target, 1, move, dir); // update pattern count on new state
    }
}

Evaluator::Evaluator(Board * board) : m_boardMap(board) {
    this->reset();
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
    } else if (board().moveCounts(Player::None) == 0) {
        board().m_winner = Player::None;
        board().m_curPlayer = Player::None;
        return true;
    } else {
        return false;
    }
}

void Evaluator::syncWithBoard(const Board& board) {
    int i;
    for (i = 0; i < board.m_moveRecord.size(); ++i) {
        if (i < this->board().m_moveRecord.size()) {
            if (this->board().m_moveRecord[i] == board.m_moveRecord[i]) {
                continue;
            } else {
                revertMove(this->board().m_moveRecord.size() - i); // 回退到首次不同步的状态
            }
        } 
        applyMove(board.m_moveRecord[i]);
    }
    revertMove(this->board().m_moveRecord.size() - i); // 回退掉多余手
}

void Evaluator::reset() {
    m_boardMap.reset();
    for (auto& scores : m_scores) {
        scores.resize(BOARD_SIZE), scores.setZero();
    }
    for (auto& density : m_density) {
        density.resize(BOARD_SIZE), density.setZero();
    }
    for (auto& distribution : m_patternDist) {
        distribution.fill(Record{}); // 最后一个元素用于总计数
    }
    for (auto& distribution : m_compoundDist) {
        distribution.fill(Record{}); // 最后一个元素用于总计数
    }
}

void Evaluator::Record::set(int delta, Player player) {
    unsigned offset = 4*sizeof(field)*Group(player);
    field += delta << offset;
}

void Evaluator::Record::set(int delta, Player favour, Player perspective, Direction dir) {
    unsigned group = Group(favour, perspective);
    unsigned offset = 4 * group + int(dir);
    field = field & ~(1u << offset) | ((delta == 1) << offset);
}

bool Evaluator::Record::get(Player favour, Player perspective, Direction dir) const {
    unsigned group = Group(favour, perspective);
    unsigned offset = 4 * group + int(dir);
    return (field >> offset) & 1;
}

unsigned Evaluator::Record::get(Player favour, Player perspective) const {
    unsigned group = Group(favour, perspective);
    return (field >> 4*group) & 0b1111; 
}

unsigned Evaluator::Record::get(Player player) const {
    unsigned offset = 4*sizeof(field)*Group(player);
    return (field >> offset) & ((1 << 4*sizeof(field)) - 1);
}

bool Evaluator::Compound::Test(Evaluator& ev, Position pose, Player player) {
    unsigned bit_field = 0; // 下面是个人感觉较妙的算法……
    for (auto comp_type : Components) {
        // bit_field汇总了四个方向的L3/D3/L2设置情况。
        // 由于位或的特性，一条线上同时有LiveTwo与DeadThree时，只会计入一次，规避了BUG。
        bit_field |= ev.m_patternDist[pose][comp_type].get(player, player);
    }
    // is-power-of-2算法，快速判断是否有>=2个bit位为1（>=2两方向有D2或LD3）
    return bit_field & (bit_field - 1);
}

void Evaluator::Compound::Update(Evaluator& ev, int delta, Position pose, Player player) {
    // 准备数据
    enum State { S0, L2, LD3, To33, To43, To44 } state = S0; // 自动机的状态定义
    array<Pattern::Type, 4> components; components.fill(Pattern::Type(-1)); // -1代表该方向上没有匹配到有效模式。
    bool restricted = false; // 为true则代表对手想要反制，只能堵在最关键点。
    // 自动机范式编程
    for (auto dir : Directions) {
        // 准备转移条件
        int cond = S0;
        for (auto comp_type : Components) {
            if (ev.m_patternDist[pose][comp_type].get(player, player, dir)) {
                switch (comp_type) {
                    case Pattern::LiveThree:
                        restricted = true;
                    case Pattern::DeadThree:
                        cond = LD3; break;
                    case Pattern::LiveTwo:
                        cond = L2; break;
                }
                components[int(dir)] = comp_type;
                break; // 找到第一个符合条件的Pattern就退出。因此Pattern有优先级之分。
            }
        }

        // 没有在该方向上匹配到模式时，状态无需转移
        if (cond == S0) continue;

        // 状态转移
        switch (int offset; state) {           
            case S0:                           //                    To44(5)
                offset = 0; goto Transfer;     //                 ↗   ↑
            case L2: case LD3:                 //          LD3(2) → To33(4)  状态转移图（有缩减）
                offset = 1; goto Transfer;     //        ↗       ↗   ↑
            case To33: case To43: case To44:    //  S0(0) → L2(1) → To22(3)
                restricted = true;
                offset = (state == To44 ? -cond : -1); // To44为最终状态，只进不出
            Transfer: 
                state = State(state + cond + offset);
        }
    }
    // 正式更新
    auto type = Compound::Type(state - State::To33);
    //ev.m_compoundDist.back()[type].set(delta, player); // 更新总计数
    for (int i = 0; i < 4; ++i) { // 按方向搜索更新计数
        if (components[i] == -1) {
            continue;
        }
        auto dir = Direction(i);
        // 更新己方分数与计数
        //ev.m_compoundDist[pose][type].set(delta, player, player, dir);
        //ev.scores(player, player)[pose] += delta * 4 * Compound::BaseScore; // 4为奖励因子
        assert(ev.scores(player, player)[pose] >= 0);
        // 更新对方反制分数
        if (restricted) { // 只更新关键点（pose）
            //ev.m_compoundDist[pose][type].set(delta, player, -player, dir);
            //ev.scores(player, -player)[pose] += delta * 4 * Compound::BaseScore;
            assert(ev.scores(player, -player)[pose] >= 0);
        } else { // 更新所有可反制这条线上的棋型的空点。最终关键点分数至少为非关键点的4倍。
            // 为防止溢出，采用试探性自增/减方式更新
            for (int sgn : { 1, -1 }) { // ds与dx, dy意思差不多……
                Position current(pose);
                for (int offset = 0; offset < TARGET_LEN / 2; ++offset) {
                    // 即使出错了，在MCTS或AlphaBeta协助下也能很快被软剪枝掉
                    if (ev.m_patternDist[current][components[i]].get(player, -player, dir)) {
                        //ev.m_compoundDist[current][type].set(delta, player, -player, dir);
                        //ev.scores(player, -player)[current] += delta * Compound::BaseScore; // 无奖励因子
                        assert(ev.scores(player, -player)[current] >= 0);
                    }
                    auto [x, y] = current; auto [dx, dy] = *dir;
                    if ((x + sgn*dx >= 0 && x + sgn*dx < WIDTH) && 
                        (y + sgn*dy >= 0 && y + sgn*dy < HEIGHT)) {
                        current = Shift(current, sgn, dir);
                    } else {
                        break;
                    }
                }
            }
        }
    }
}

// 以下模式被设计为没有互为前缀码的情况。
PatternSearch Evaluator::Patterns = {
    { "+xxxxx",    Pattern::Five,      9999 },
    { "-_oooo_",   Pattern::LiveFour,  9000 },
    { "-xoooo_",   Pattern::DeadFour,  2500 },
    { "-o_ooo",    Pattern::DeadFour,  3000 },
    { "-oo_oo",    Pattern::DeadFour,  2600 },
    { "-~_ooo_~",  Pattern::LiveThree, 3000 },
    { "-x^ooo_~",  Pattern::LiveThree, 2900 },
    { "-^o_oo^",   Pattern::LiveThree, 2800 },
    { "-xooo__~",  Pattern::DeadThree, 510 },
    { "-xoo_o_~",  Pattern::DeadThree, 520 },
    { "-xoo__o~",  Pattern::DeadThree, 520 },
    { "-xo_oo_~",  Pattern::DeadThree, 530 },
    { "-xo__oo",   Pattern::DeadThree, 530 },
    { "-xooo__x",  Pattern::DeadThree, 500 },
    { "-xoo_o_x",  Pattern::DeadThree, 500 },
    { "-xoo__ox",  Pattern::DeadThree, 500 },
    { "-xo_oo_x",  Pattern::DeadThree, 500 },
    { "-x_ooo_x",  Pattern::DeadThree, 500 },
    { "-~oo__o~",  Pattern::DeadThree, 750 },
    { "-oo__oo",   Pattern::DeadThree, 540 },
    { "-o_o_o",    Pattern::DeadThree, 550 },
    { "-~oo__~",   Pattern::LiveTwo,   650 },
    { "-~_o_o_~",  Pattern::LiveTwo,   600 },
    { "-x^o_o_^",  Pattern::LiveTwo,   550 },
    { "-^o__o^",   Pattern::LiveTwo,   550 },
    { "-xoo___",   Pattern::DeadTwo,   150 },
    { "-xo_o__",   Pattern::DeadTwo,   250 },
    { "-xo__o_",   Pattern::DeadTwo,   200 },
    { "-o___o",    Pattern::DeadTwo,   180 },
    { "-x_oo__x",  Pattern::DeadTwo,   100 },
    { "-x_o_o_x",  Pattern::DeadTwo,   100 },
    { "-~o___~",   Pattern::LiveOne,   150 },
    { "-x~_o__^",  Pattern::LiveOne,   140 },
    { "-x~__o_^",  Pattern::LiveOne,   150 },
    { "-xo___~",   Pattern::DeadOne,   30 },
    { "-x_o___x",  Pattern::DeadOne,   40 },
    { "-x__o__x",  Pattern::DeadOne,   50 },
};

tuple<Array<int, BLOCK_SIZE, BLOCK_SIZE, RowMajor>, int> Evaluator::BlockWeights = []() {
    tuple_element_t<0, decltype(BlockWeights)> weight;
    tuple_element_t<1, decltype(BlockWeights)> score = 10;
    weight << 2, 0, 0, 1, 0, 0, 2,
              0, 4, 3, 3, 3, 4, 0,
              0, 3, 5, 4, 5, 3, 0,
              1, 3, 4, 0, 4, 3, 1,
              0, 3, 5, 4, 5, 3, 0,
              0, 4, 3, 3, 3, 4, 0,
              2, 0, 0, 1, 0, 0, 2;
    return make_tuple(weight, score);
}();

const Pattern::Type Evaluator::Compound::Components[3] = { 
    Pattern::LiveThree, Pattern::DeadThree, Pattern::LiveTwo 
};
const int Evaluator::Compound::BaseScore = 800;

}