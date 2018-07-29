#include "Pattern.h"
#include "utils/ACAutomata.h"
#include <iostream>
#include <bitset>
#include <future>

using namespace std;
using namespace Eigen;

namespace Gomoku {

/* ------------------- Pattern类实现 ------------------- */

Pattern::Pattern(std::string_view proto, Type type, int score) 
    : str(proto.begin() + 1, proto.end()), score(score), type(type),
      favour(proto[0] == '+' ? Player::Black : Player::White) {

}

/* ------------------- PatternSearch类实现 ------------------- */

bool PatternSearch::HasCovered(const Entry& entry, size_t pose) {
    const auto [pattern, offset] = entry;
    return 0 <= offset - pose && offset - pose < pattern.str.length();
}

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
        if (state == ref->m_invariants[code]) { // 判断当前状态在接受code后是否不发生转移（即「不动点」状态）
            while (!target.empty() && target[0] == code) { // 快速跳转冗长的无转移状态
                ++offset, target.remove_prefix(1); 
            }
            continue;
        }
        int next = ref->m_base[state] + code; // 获取下一状态的索引
        if (ref->m_check[next] == state) { // 尝试往下匹配target
            state = next; // 匹配成功转移
        } else if (state != 0) { // 匹配失败时的判断。若最终在根节点匹配失败，则跳过该字符继续搜索
            state = ref->m_fail[state]; // 匹配失败转移
            continue; // 失败转移后在下一循环再次尝试匹配
        }
        ++offset, target.remove_prefix(1);
    } while (ref->m_check[ref->m_base[state]] != state); // 发现叶子结点时，暂时中断匹配
    return *this;
}

// 解析当前成功匹配到的模式
PatternSearch::Entry PatternSearch::generator::operator*() const {
    const auto leaf = ref->m_base[state];
    return { ref->m_patterns[-ref->m_base[leaf]], offset };
}

PatternSearch::generator PatternSearch::execute(string_view target) {
    return generator{ target, this };
}

vector<PatternSearch::Entry> PatternSearch::matches(string_view target) {
    vector<Entry> entries;
    for (auto record : execute(target)) { 
        entries.push_back(record); 
    }
    return entries;
}

/* ------------------- Evaluator::Updater类实现 ------------------- */

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

template <int Size = BLOCK_SIZE, typename Array_t, typename value_t = typename Array_t::value_type>
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

void Evaluator::Updater::reset(int delta, Position move, Player player) {
	this->delta = delta;
    this->move = move;
	this->player = player;
    this->compound_keys.clear();
    this->compounds.clear();
}

Compound* Evaluator::Updater::findCompound(Position pose, Player player) {
    for (int i = 0; i < compound_keys.size(); ++i) {
        if (compound_keys[i] == make_tuple(pose, player)) {
            return &compounds[i];
        }
    }
    return nullptr;
}

void Evaluator::Updater::matchPatterns(Direction dir) {
    matchResults(delta, dir).clear();
    auto target = ev.m_boardMap.lineView(move, dir);
    for (auto&& entry : Patterns.execute(target)) {
        if (PatternSearch::HasCovered(entry, TARGET_LEN / 2)) {
            matchResults(delta, dir).push_back(std::move(entry));
        }
    }
}

void Evaluator::Updater::updatePatterns(Direction dir) {
    for (const auto [pattern, offset] : matchResults(delta, dir)) {
        if (pattern.type == Pattern::Five) {
            // 此前board已完成applyMove，故此处设置curPlayer为None不会发生阻塞。
            ev.board().m_curPlayer = Player::None;
            ev.board().m_winner = pattern.favour;
            continue;
        }
        auto current = Shift(move, offset - TARGET_LEN / 2, dir);
        ev.m_patternDist.back()[pattern.type].set(delta, pattern.favour); // 修改总计数
        for (int i = 0; i < pattern.str.length(); ++i, SelfShift(current, -1, dir)) { // 修改空位数据
            const auto piece = pattern.str.rbegin()[i];
            if (piece == '_' || piece == '^') { // '_'代表己方有效空位，'^'代表对方反制空位 
                const auto multiplier = (dir == Direction::LeftDiag || dir == Direction::RightDiag ? 1.2 : 1);
                const auto score = int(delta * multiplier * pattern.score);
                const auto update_pose = [&](Player perspective) {
                    ev.m_patternDist[current][pattern.type].set(delta, pattern.favour, perspective, dir);
                    ev.scores(pattern.favour, perspective)[current] += score;
                    assert(ev.scores(pattern.favour, perspective)[current] >= 0);
                };
                switch (piece) { // 利用了switch的穿透特性
                    case '_': update_pose(pattern.favour);
                    case '^': update_pose(-pattern.favour);
                }
            }
        }
    }
}

void Evaluator::Updater::updateCompound(Direction dir) {
#if true
	const auto view = ev.m_boardMap.lineView(move, dir);
	for (const auto player : { Player::White, Player::Black }) {
		auto current = Position::npos;
		auto offset = 0;
		for (int i = 0; i < view.size(); ++i) {
			if (view[i] != EncodeCharset('-')) {
				continue;
			} else if (current == Position::npos) {
				current = Shift(move, i - TARGET_LEN / 2, dir);
			} else {
				SelfShift(current, i - offset, dir);
			}
			offset = i;
			if (ev.density(player)[0][current] < 2) {
				continue; // 周围至少要有2个同色子
			} 
			if (findCompound(current, player) != nullptr) {
				continue; // 若找到复合模式记录，则代表已更新过，直接跳过
			} 
			if (Compound::Test(ev, current, player)) {
				compound_keys.emplace_back(current, player);
				compounds.emplace_back(ev, current, player);
				compounds.back().update(delta);
			}
			if (view[i] == EncodeCharset('?')) {
				break;
			}
		}
	}
#elif
    for (auto [pattern, offset] : matchResults(delta, dir)) {
        auto current = Shift(move, offset - TARGET_LEN / 2, dir);
        for (int i = 0; i < pattern.str.length(); ++i, SelfShift(current, -1, dir)) {
            const auto piece = pattern.str.rbegin()[i];
            if (piece != '_') {
				continue; // 只允许关键空位通过
            }
			switch (pattern.type) {
			case Pattern::DeadOne:
				continue; // 直接过滤
			case Pattern::LiveOne:
				// 活一不会对对方的复合模式造成影响
				if (pattern.favour != player) {
					continue; 
				}
				// 此时，若下棋不能delta == 1，若悔棋不能delta == -1
				if ((delta == 1) ^ (player == Player::None)) {
					continue; // 即：仅当活一的棋子不是updater.move时才能通过
				}
				continue;
				break;
			case Pattern::DeadFour:
				continue;
			}
			if (findCompound(current, pattern.favour) != nullptr) {
				continue; // 若找到复合模式记录，则代表已更新过，直接跳过
			}
			if (Compound::Test(ev, current, pattern.favour)) {
				compound_keys.emplace_back(current, pattern.favour);
				compounds.emplace_back(ev, current, pattern.favour);
				compounds.back().update(delta);
			}
        }
    }
#endif
}

void Evaluator::Updater::updateBlock(int delta, Player src_player) {
    // 数据准备
    const auto sign = [](int x) { return x < 0 ? -1 : 1; };
    const auto mask = [](int x) { return x > 0 ? 1 : 0; };
    auto [weights, score] = BlockWeights;
    auto base_weights  = BlockView(weights, move);
    auto count_block   = BlockView(ev.density(src_player)[0], move);
    auto weight_block  = BlockView(ev.density(src_player)[1], move);
    auto score_block   = BlockView(ev.scores(src_player, src_player), move); // 只为关键矩阵更新分数
    auto sign_block    = weight_block.unaryExpr(sign); // 表征某位置是否有棋的{-1, 1}二值矩阵（0以上认为没有棋）
    auto mask_block    = weight_block.unaryExpr(mask).eval(); // 表征某位置是否有有效计数的{0, 1}二值矩阵（0与负数认为是无效计数）

    // 更新density（sign对应原位置是否有棋，delta对应下棋还是悔棋）。
    weight_block += sign_block * delta * base_weights;
    count_block  += sign_block * delta * base_weights.unaryExpr(mask);
    
    // 修改中心位置的状态（是否可以用来计分）
    for (auto perspective : { Player::Black, Player::White }) 
    for (auto& count_or_weight : ev.density(perspective)) {
        switch (auto& value = count_or_weight[move]; delta) {
        case 1: // 代表这里下了一子
            value *= -1; // 反转计数，表明这个位置已有棋子占用
            value -= 1;  // 减去一辅助数，保证value一定是负数
            break;
        case -1: // 代表悔了这一子
            value += 1;  // 除去之前减掉的辅助数
            value *= -1; // 反转计数，表明这里可以用来计分了
            break; 
        }
    }

    // 将mask_block更新成"delta_block"，表征有效计数的变化，并以此修改score_block
    score_block += score * (weight_block.unaryExpr(mask) - mask_block); // score_block在有有效计数的位置上增加一个固定的分数
    if (auto count = ev.density(-src_player)[0][move]; (count != 0 && count != -1)) { // count为0或-1代表原位置最初没有计分，故不修改分数
        ev.scores(-src_player, -src_player)[move] -= delta * score; // 对方的分数在move处变化
    }
}

void Evaluator::Updater::updateMove(Position move, Player src_player) {
    this->reset(-1, move, src_player); // 撤销旧状态
	for (auto dir : Directions) {
		matchPatterns(dir);
    }
    for (auto dir : Directions) {
        updateCompound(dir);
    }
	for (auto dir : Directions) {
        updatePatterns(dir);
    }
    if (src_player != Player::None) {
        ev.m_boardMap.applyMove(move);
        updateBlock(1, src_player);
    } else {
        ev.m_boardMap.revertMove();
        updateBlock(-1, ev.board().m_curPlayer);
    }
    this->reset(1, move, src_player); // 更新新状态
	for (auto dir : Directions) {
        matchPatterns(dir);
    }
	for (auto dir : Directions) {
        updatePatterns(dir);
    }
    for (auto dir : Directions) {
        updateCompound(dir);
    }
}

/* ------------------- Evaluator类实现 ------------------- */

Evaluator::Evaluator(Board * board) : m_updater(*this), m_boardMap(board) {
    this->reset();
}

Player Evaluator::applyMove(Position move) {
    if (board().m_curPlayer != Player::None && board().checkMove(move)) {
        m_updater.updateMove(move, board().m_curPlayer);
    }
    for (int i = 0; i < BOARD_SIZE; ++i) {
        if (!board().moveState(Player::None, i)) {
            for (int j = 0; j < 4; ++j) {
                if (m_scores[j][i] != 0) {
                    auto b = board().toString();
					for (auto pose : board().m_moveRecord) {
						std::cout << std::to_string(pose) << std::endl;
					}
                    throw b;
                }
            }
        } else {
            for (int j = 0; j < 4; ++j) {
                if (m_scores[j][i] < 0) {
                    auto b = board().toString();
                    throw b;
                }
            }
        }
    }
    return board().m_curPlayer;
}

Player Evaluator::revertMove(size_t count) {
    for (auto i = 0; i < count && !board().m_moveRecord.empty(); ++i) {
        m_updater.updateMove(board().m_moveRecord.back(), Player::None);
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
    int i = 0;
    for (; i < board.m_moveRecord.size(); ++i) {
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
    for (auto& density : m_density) 
    for (auto& cnt_n_wt : density) { // count & weight
        cnt_n_wt.resize(BOARD_SIZE), cnt_n_wt.setZero();
    }
    for (auto& distribution : m_patternDist) {
        distribution.fill(Record{}); // 最后一个元素用于总计数
    }
    for (auto& distribution : m_compoundDist) {
        distribution.fill(Record{}); // 最后一个元素用于总计数
    }
}

/* ------------------- Evaluator::Record类实现 ------------------- */

void Evaluator::Record::set(int delta, Player player) {
    unsigned offset = 4*sizeof(field)*Group(player);
    field += delta << offset;
}

void Evaluator::Record::set(int delta, Player favour, Player perspective, Direction dir) {
    unsigned group = Group(favour, perspective), offset = (4*group + int(dir))*2;
    unsigned lower = 1 << offset, higher = lower << 1, mask = higher | lower;
    unsigned value = (delta == 1 ? (field << 1) | lower : (field >> 1) & ~higher);
    field = (field & ~mask) | (value & mask);
}

unsigned Evaluator::Record::get(Player favour, Player perspective, Direction dir) const {
    unsigned group = Group(favour, perspective);
    unsigned offset = (4*group + int(dir))*2;
    return (field >> offset) & 0b11;
}

unsigned Evaluator::Record::get(Player favour, Player perspective) const {
    unsigned group = Group(favour, perspective);
    return (field >> 8*group) & 0xff; 
}

unsigned Evaluator::Record::get(Player player) const {
    unsigned offset = 4*sizeof(field)*Group(player);
    return (field >> offset) & ((1 << 4*sizeof(field)) - 1);
}

/* ------------------- Compound类实现 ------------------- */

constexpr const Pattern::Type CompTypes[] = {
    Pattern::LiveThree, Pattern::DeadThree, Pattern::LiveTwo
};

bool Compound::Test(Evaluator& ev, Position pose, Player player) {
    unsigned bit_field = 0; 
    for (const auto comp_type : CompTypes) {
        // bit_field汇总了四个方向的L3/D3/L2设置情况。
        // 由于位或的特性，一条线上最多只记录一种模式类型，规避了BUG。
        bit_field |= ev.m_patternDist[pose][comp_type].get(player, player);
    }
    // is-power-of-2算法，快速判断是否有>=2个bit位为1
    return bit_field & (bit_field - 1);
}

Compound::Compound(Evaluator& ev, Position pose, Player favour) 
    : ev(ev), position(pose), favour(favour) {
    this->locate();
}

void Compound::locate() {
    enum State { S0, L2, LD3, To33, To43, To44 } state = S0;
    for (auto comp_dir : Directions) {
        // 准备转移条件
        int cond = S0; // 初始转移条件
        int count = 0; // 当前方向上模式计数
        for (auto comp_type : CompTypes) {
            switch (ev.m_patternDist[position][comp_type].get(favour, favour, comp_dir)) {
                case 0b00: count = 0; break;
                case 0b01: count = 1; break;
                case 0b11: count = 2; break;
            }
            if (count != 0) {
                switch (comp_type) {
                case Pattern::LiveThree:
                    l3_count += 1;
                case Pattern::DeadThree:
                    cond = LD3; break;
                case Pattern::LiveTwo:
                    cond = L2;  break;
                }
                components.insert(components.end(), count, { comp_dir, comp_type });
                break; // 找到第一个符合条件的Pattern就退出。因此Pattern有优先级之分。
            }
        }

        // 没有在该方向上匹配到模式时，状态无需转移
        if (cond == S0) continue;

        // 状态转移
        for (int i = 0; i < count; ++i) {
            switch (int offset; state) {
            case S0:                           //                    To44(5)
                offset = 0; goto Transfer;     //                 ↗   ↑
            case L2: case LD3:                 //          LD3(2) → To43(4)  状态转移图（有缩减）
                offset = 1; goto Transfer;     //        ↗       ↗   ↑
            case To33: case To43: case To44:    //  S0(0) → L2(1) → To33(3)
                triple_cross = true;
                offset = (state == To44 ? -cond : -1); // To44为最终状态，只进不出
            Transfer:
                state = State(state + cond + offset);
            }
        }
    }
    type = Compound::Type(state - State::To33);
    count = bitset<8>(ev.m_compoundDist[position][type].get(favour, favour)).count();
}

void Compound::update(int delta) {
    for (auto base : components) {
        // 前置转移处理
        switch (2 * count + delta) {
            case -1: // 0 <-> -1，该转移不存在，状态不变
                return;
        }

        // 更新复合模式的关键点
        updateCritical(delta, base);

        // 更新复合模式的非关键反击点
        if (!triple_cross && l3_count == 0) {
            updateAntis(delta, base);
        }

        // 后置转移处理
        switch (2 * count + delta) {
            case 3:  // 1 <-> 2，该转移关系到复合模式的存在性
                ev.m_compoundDist.back()[type].set(delta, favour); // 更新复合模式总计数
        }

        // 状态正式转移
		count += delta;
    }
}

void Compound::updateCritical(int delta, Component component) {
    updatePose(delta, position, component, favour); // 更新self_worthy
    updatePose(delta, position, component, -favour); // 更新rival_anti
}

void Compound::updateAntis(int delta, Component component) {
    const auto [comp_dir, comp_type] = component;
    if (gen_dir != comp_dir) {
        auto target = ev.m_boardMap.lineView(position, comp_dir);
        generator = ev.Patterns.execute(target);
        gen_dir = comp_dir;
    }
    for (auto [pattern, offset] : generator) {
        if (pattern.type == comp_type // 必须是：①.模式类型为comp.type
              && PatternSearch::HasCovered({ pattern, offset }) // ②.模式必须覆盖了position
              && pattern.str.rbegin()[offset - TARGET_LEN / 2] == '_') { // ③.position是关键点'_'的模式才行
            // 遍历pattern寻找空位，更新额外的rival_anti分数
            auto current = Shift(position, offset - TARGET_LEN / 2, gen_dir);
            for (int i = 0; i < pattern.str.length(); ++i, SelfShift(current, -1, gen_dir)) {
                auto piece = pattern.str.rbegin()[i];
                if ((piece == '_' || piece == '^') && current != position) { // 不更新关键点
                    // 即使棋型评估出错了，在MCTS或AlphaBeta协助下也能很快被软剪枝掉
                    updatePose(delta, current, component, -favour);
                }
            }
            break; // 一次只更新一个Pattern
        }
    }
}

void Compound::updatePose(int delta, Position pose, Component component, Player perspective) {
    const auto comp_dir = std::get<0>(component);
    ev.m_compoundDist[pose][type].set(delta, favour, perspective, comp_dir);
    ev.scores(favour, perspective)[pose] += delta * Compound::BaseScore;
    assert(ev.scores(favour, perspective)[pose] >= 0);
}

/* ------------------- 数据区 ------------------- */

PatternSearch Evaluator::Patterns = {
    { "+xxxxx",    Pattern::Five,      9999 },
    { "-_oooo_",   Pattern::LiveFour,  9000 },
    { "-xoooo_",   Pattern::DeadFour,  2500 },
    { "-o_ooo",    Pattern::DeadFour,  3000 },
    { "-oo_oo",    Pattern::DeadFour,  2600 },
    { "-~_ooo_~",  Pattern::LiveThree, 3000 },
    { "-x^ooo_~",  Pattern::LiveThree, 2900 },
    { "-~o_oo~",   Pattern::LiveThree, 2800 },
    { "-~o~oo_~",  Pattern::DeadThree, 1400 },
    { "-~oo~o_~",  Pattern::DeadThree, 1200 },
    { "-x_o~oo~",  Pattern::DeadThree, 1300 },
    { "-x_oo~o~",  Pattern::DeadThree, 1100 },
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
    { "-xo_o__",   Pattern::DeadTwo,   160 },
    { "-xo__o_",   Pattern::DeadTwo,   170 },
    { "-o___o",    Pattern::DeadTwo,   180 },
    { "-x_oo__x",  Pattern::DeadTwo,   120 },
    { "-x_o_o_x",  Pattern::DeadTwo,   120 },
    { "-~o___~",   Pattern::LiveOne,   150 },
    { "-x~_o__^",  Pattern::LiveOne,   140 },
    { "-x~__o_^",  Pattern::LiveOne,   150 },
    { "-xo___~",   Pattern::DeadOne,   30 },
    { "-x_o___x",  Pattern::DeadOne,   40 },
    { "-x__o__x",  Pattern::DeadOne,   50 },
};

tuple<Array<int, BLOCK_SIZE, BLOCK_SIZE, RowMajor>, int> Evaluator::BlockWeights = []() {
    tuple_element_t<0, decltype(BlockWeights)> weight;
    tuple_element_t<1, decltype(BlockWeights)> score = 160;
    weight << 2, 0, 0, 1, 0, 0, 2,
              0, 4, 3, 3, 3, 4, 0,
              0, 3, 5, 4, 5, 3, 0,
              1, 3, 4, 0, 4, 3, 1,
              0, 3, 5, 4, 5, 3, 0,
              0, 4, 3, 3, 3, 4, 0,
              2, 0, 0, 1, 0, 0, 2;
    return make_tuple(weight, score);
}();

const int Compound::BaseScore = 600;

}
