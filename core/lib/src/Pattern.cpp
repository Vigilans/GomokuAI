#include "Pattern.h"
#include <numeric>
#include <queue>
#include <set>

using namespace std;
using Eigen::Array;

namespace Gomoku {

constexpr Direction Directions[] = {
    Direction::Horizontal, Direction::Vertical, Direction::LeftDiag, Direction::RightDiag
};

/* ------------------- Pattern类实现 ------------------- */

constexpr int Codeset[] = { 1, 2, 3, 4 };

constexpr int EncodeCharset(char ch) {
    switch (int code = 0; ch) {
        case '-': case '_': case '^': case '~': ++code;
        case '?': ++code;
        case 'o': ++code;
        case 'x': ++code;
        default: return code;
    }
}

Pattern::Pattern(std::string_view proto, Type type, int score) 
    : str(proto.begin() + 1, proto.end()), score(score), type(type),
      favour(proto[0] == '+' ? Player::Black : Player::White) {

}

/* ------------------- PatternSearch类实现 ------------------- */

class AhoCorasickBuilder {
public:
    // 用于构建双数组的临时结点
    struct Node {
        int code = 0, depth = 0, first = 0; mutable int last = first + 1; // 默认构造时生成根节点
        bool operator<(Node other) const { // 采用depth->first字典序比较。last不参与比较，故可声明为mutable。
            return std::tie(depth, first) < std::tie(other.depth, other.first); 
        }
    };

    AhoCorasickBuilder(initializer_list<Pattern> protos) 
        : m_tree{ Node{} }, m_patterns(protos) {

    }

    void build(PatternSearch* searcher) {
        this->reverseAugment()->flipAugment()->boundaryAugment()->sortPatterns()
            ->buildNodeBasedTrie()->buildDAT(searcher)->buildACGraph(searcher);
    }

private:
    // 不对称的pattern反过来看与原pattern等价
    AhoCorasickBuilder* reverseAugment() {
        for (int i = 0, size = m_patterns.size(); i < size; ++i) {
            Pattern reversed(m_patterns[i]);
            std::reverse(reversed.str.begin(), reversed.str.end());
            if (reversed.str != m_patterns[i].str) {
                m_patterns.push_back(std::move(reversed));
            }
        }
        return this;
    }

    // pattern的黑白颜色对调后与原pattern等价
    AhoCorasickBuilder* flipAugment() {
        for (int i = 0, size = m_patterns.size(); i < size; ++i) {
            Pattern flipped(m_patterns[i]);
            flipped.favour = -flipped.favour;
            for (auto& piece : flipped.str) {
                if (piece == 'x') piece = 'o';
                else if (piece == 'o') piece = 'x';
            }
            m_patterns.push_back(std::move(flipped));
        }
        return this;
    }

    // 被边界堵住的pattern与被对手堵住一角的pattern等价
    AhoCorasickBuilder* boundaryAugment() {
        for (int i = 0, size = m_patterns.size(); i < size; ++i) {
            char enemy = m_patterns[i].favour == Player::Black ? 'x' : 'o';
            auto first = m_patterns[i].str.find_first_of(enemy);
            auto last  = m_patterns[i].str.find_last_of(enemy);
            if (first != string::npos) { // 最多只有三种情况
                Pattern bounded(m_patterns[i]);
                bounded.str[first] = '?';
                m_patterns.push_back(std::move(bounded));
                if (last != first) {
                    bounded.str[last] = '?';
                    m_patterns.push_back(std::move(bounded));
                    bounded.str[first] = enemy;
                    m_patterns.push_back(std::move(bounded));
                }
            }
        }
        return this;
    }

    // 根据编码字典序排序patterns
    AhoCorasickBuilder* sortPatterns() {
        // 根据最终的patterns数量预留空间
        vector<int> codes(m_patterns.size());
        vector<int> indices(m_patterns.size());
        
        // 编码、填充与排序
        std::transform(m_patterns.begin(), m_patterns.end(), codes.begin(), [](const Pattern& p) {
            return std::reduce(p.str.begin(), p.str.end(), 0, [](int sum, char ch) { 
                return sum *= std::size(Codeset), sum += EncodeCharset(ch); 
            }); // 通过基数排序间接实现对patterns的字典序排序
        }); 
        std::iota(indices.begin(), indices.end(), 0);
        std::sort(indices.begin(), indices.end(), [&codes](int lhs, int rhs) { 
            return codes[lhs] < codes[rhs]; 
        });

        // 根据indices重排m_patterns
        vector<Pattern> medium;
        medium.reserve(m_patterns.size());
        for (auto index : indices) {
            medium.push_back(std::move(m_patterns[index]));
        }
        m_patterns.swap(medium);
        return this;
    }

    // DFS插入生成基于结点的Trie树
    AhoCorasickBuilder* buildNodeBasedTrie() {
        using NodeIter = decltype(m_tree)::iterator;
        // suffix为以parent的子结点为起点，直至整个pattern结束的后缀
        function<void(string_view, NodeIter)> insert_pattern = [&](string_view suffix, NodeIter parent) {
            if (!suffix.empty()) {
                Node key = { // 准备子结点索引
                    EncodeCharset(suffix[0]), 
                    parent->depth + 1, 
                    parent->first, parent->last 
                }; 
                auto child = m_tree.find(key); // 尝试匹配该前缀结点
                if (child == m_tree.end()) { // 若前缀匹配失败，则新生成一个分支结点
                    // 由于插入时已按字典序排序，故新区间的first沿上个区间的last继续即可
                    key.first = parent->last;
                    key.last = key.first + 1;
                    child = m_tree.insert(key).first;
                }
                insert_pattern(suffix.substr(1), child); // 递归将pattern剩下的部分插入trie树中
                parent->last = child->last; // 反向传播，更新区间右边界值。由于last不影响Node的排序，故修改是可行的。
            }
        };
        auto root = m_tree.find({});
        for (auto pattern : m_patterns) {
            insert_pattern(pattern.str, root); // 按字典序插入pattern
        }
        return this;
    }

    // DFS遍历生成双数组Trie树
    AhoCorasickBuilder* buildDAT(PatternSearch* ps) {
        using NodeIter = decltype(m_tree)::iterator;
        // index为node在双数组中的索引，之前的递归中已确定好
        function<void(int, NodeIter)> build_recursive = [&](int index, NodeIter node) {
            if (node->last - node->first <= 1) {
                // Base case: 已抵达叶结点，设置index的base为(-对应pattern表的下标)
                ps->m_base[index] = -node->first;
            } else {
                // 准备子节点（利用[first, last)迭代器区间框定）与begin变量
                auto first = m_tree.lower_bound({ 0, node->depth + 1, node->first }); // 子节点下界（no less than）
                auto last = m_tree.upper_bound({ 0, node->depth + 1, node->last - 1 }); // 子节点上界（greater than）
                int begin; // 待寻找的当前结点的base值

                /*
                    初始条件：begin从根节点0开始
                    退出条件：找到一个可以容纳index结点的所有子结点的begin值
                    状态转移：沿前向链表找到下一个空位
                    注意：根节点由于没有父结点，故其无本义check值，该位置用来作为前向链表的起点。
                    参考：http://www.aclweb.org/anthology/D13-1023  
                */
                for (begin = 0; ; begin = -ps->m_check[begin]) {
                    // 空间不足时扩充m_base与m_check数组。阈值设为size-1以保证最后一位为空
                    if (begin + std::size(Codeset) >= ps->m_check.size() - 1) {
                        // 扩充空间
                        auto pre_size = ps->m_base.size();
                        ps->m_base.resize(2*pre_size + 1);
                        ps->m_check.resize(2*pre_size + 1);
                        // 填充下标补全双链表
                        for (int i = pre_size; i < ps->m_base.size(); ++i) {
                            ps->m_base[i] = -(i-1); // 逆向链表
                            ps->m_check[i] = -(i+1); // 前向链表
                        }
                    }

                    if (std::all_of(first, last, [&](const Node& node) {
                        return ps->m_check[begin + node.code] <= 0;
                    })) break;
                }

                // 遍历子结点，设置相关状态后对子结点递归构建
                for (auto cur = first; cur != last; ++cur) {
                    // 取得当前子节点下标
                    int c_i = begin + cur->code;

                    // 将当前下标移出空闲节点（利用Dancing Links）
                    ps->m_check[-ps->m_base[c_i]] = ps->m_check[c_i];
                    ps->m_base[-ps->m_check[c_i]] = ps->m_base[c_i];

                    // 将子结点check值与父结点绑定，对子节点递归
                    ps->m_check[c_i] = index;
                    build_recursive(c_i, cur);
                }
                ps->m_base[index] = begin;
            }
        };
        auto root = m_tree.find({});
        build_recursive(0, root);
        ps->m_patterns.swap(m_patterns);
        return this;
    }

    // BFS遍历，为DAT构建AC自动机的fail指针数组
    AhoCorasickBuilder* buildACGraph(PatternSearch* ps) {
        // 初始，所有结点的fail指针都指向根节点
        ps->m_fail.resize(ps->m_base.size(), 0);

        // 准备好结点队列，置入根节点作为初始值
        queue<int> node_queue;
        node_queue.push(0); 
        while (!node_queue.empty()) {
            // 取出当前结点 
            int cur_node = node_queue.front();
            node_queue.pop();

            // 准备新的结点
            for (auto code : Codeset) {
                int child_node = ps->m_base[cur_node] + code;
                if (ps->m_check[child_node] == cur_node) {
                    node_queue.push(child_node);
                }
            }

            // 为当前结点设置fail指针
            int code = cur_node - ps->m_base[ps->m_check[cur_node]]; // 取得转换至cur结点的编码
            int pre_fail_node = ps->m_check[cur_node]; // 初始pre_fail结点设置为cur结点的父结点
            while (pre_fail_node != 0) { // 按匹配后缀长度从长->短不断跳转fail结点，直到长度为0（抵达根节点）
                // 每一次fail指针的跳转，最大匹配后缀的长度至少减少了1，因此循环是有限的
                pre_fail_node = ps->m_fail[pre_fail_node]; 
                int fail_node = ps->m_base[pre_fail_node] + code;
                if (ps->m_check[fail_node] == pre_fail_node) { // 如若pre_fail结点能通过code抵达某子结点（即fail_node存在）
                    ps->m_fail[cur_node] = fail_node; // 则该子结点即为cur结点的fail指针的指向
                    break;
                }
                // 若直到pre_fail结点为0才退出，则当前结点的fail指针指向根节点。
            }
            // NOTE: 由于Patterns被设计为一定没有互为前缀码的情况，因此无需对"output表"（即m_patterns）进行扩充。
        }
        return this;
    }

private:
    set<Node> m_tree; // 利用在插入中保持有序的RB树，作为临时保存Trie树的结构
    vector<Pattern> m_patterns;
};

PatternSearch::PatternSearch(initializer_list<Pattern> protos) {
    AhoCorasickBuilder builder(protos);
    builder.build(this);
}

// 持续转移，直到匹配下一个成功的模式或查询结束
const PatternSearch::generator& PatternSearch::generator::operator++() {
    while (ref->m_base[state] >= 0 && !target.empty()) {
        int next = ref->m_base[state] + EncodeCharset(target[0]);
        if (ref->m_check[next] == state) { // 尝试往下匹配target
            state = next; // 匹配成功转移
        } else {
            state = ref->m_fail[state]; // 匹配失败转移
        }
        ++offset, target.remove_prefix(1);
    }
    return *this;
}

// 解析当前成功匹配到的模式
PatternSearch::Entry PatternSearch::generator::operator*() const {
    return { ref->m_patterns[-ref->m_base[state]], offset };
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

/* ------------------- BoardMap类实现 ------------------- */

constexpr tuple<int, int> BoardMap::ParseIndex(Position pose, Direction direction) {
    // 由于前与后MAX_PATTERN_LEN - 1位均为'?'（越界位），故需设初始offset
    switch (int offset = MAX_PATTERN_LEN - 1; direction) {
        case Direction::Horizontal: // 0 + x∈[0, WIDTH) | y: 0 -> HEIGHT
            return { pose.x(), offset + pose.y() };
        case Direction::Vertical:   // WIDTH + y∈[0, HEIGHT) | x: 0 -> WIDTH
            return { WIDTH + pose.y(), offset + pose.x() };
        case Direction::LeftDiag:   // (WIDTH + HEIGHT) + (HEIGHT - 1) + x-y∈[-(HEIGHT - 1), WIDTH) | min(x, y): (x, y) -> (x+1, y+1)
            return { WIDTH + 2 * HEIGHT - 1 + pose.x() - pose.y(), offset + std::min(pose.x(), pose.y()) };
        case Direction::RightDiag:  // 2*(WIDTH + HEIGHT) - 1 + x+y∈[0, WIDTH + HEIGHT - 1) | min(WIDTH - x, y): (x, y) -> (x-1, y+1)
            return { 2 * (WIDTH + HEIGHT) - 1 + pose.x() + pose.y(), offset + std::min(WIDTH - pose.x(), pose.y()) };
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
        m_lineMap[index][offset] = m_board->m_curPlayer == Player::Black ? 'x' : 'o';
    }
    return m_board->applyMove(move, false);
}

Player BoardMap::revertMove(size_t count) {
    for (int i = 0; i < count; ++i) {
        for (auto direction : Directions) {
            const auto [index, offset] = ParseIndex(m_board->m_moveRecord.back(), direction);
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
        auto [index, _] = ParseIndex(i, direction);
        m_lineMap[index].push_back('-'); // 按每个位置填充空位('-')
    }
    for (auto& line : m_lineMap) {
        line.append(MAX_PATTERN_LEN - 1, '?'); // 线后填充越界位('?')
    }
}

/* ------------------- Evaluator类实现 ------------------- */

template <size_t Length = TARGET_LEN, typename Array_t>
inline auto& LineView(Array_t& src, Position move, Direction dir) {
    static vector<Array_t::value_type*> view_ptrs(Length);
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

template <int Size = BLOCK_SIZE, typename Array_t>
inline auto BlockView(Array_t& src, Position move) {
    static Eigen::Map<Array<Array_t::value_type, HEIGHT, WIDTH>> view(nullptr);
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

void Evaluator::updateLine(string_view target, int delta, Position move, Direction dir) {
    for (auto [pattern, offset] : Patterns.execute(target)) {
        if (pattern.type == Pattern::Five) {
            // 此前board已完成applyMove，故此处设置curPlayer为None不会发生阻塞。
            board().m_curPlayer = Player::None;
            board().m_winner = pattern.favour;
        } else {
            for (int i = 0; i < pattern.str.length(); ++i) {
                auto piece = pattern.str[i];
                if (piece == '_' || piece == '^') { // '_'代表己方有效空位，'^'代表对方反制空位
                    auto pose = Shift(move, offset + i - TARGET_LEN / 2, dir);
                    auto perspective = (piece == '_' ? pattern.favour : -pattern.favour);
                    auto multiplier = (dir == Direction::LeftDiag || dir == Direction::RightDiag ? 1.2 : 1);
                    // 请善用块折叠获得更好阅读效果……
                    auto update_pattern = [&]() { 
                        m_patternDist[pose][pattern.type].set(delta, pattern.favour, perspective, dir);
                        m_patternDist.back()[pattern.type].set(delta, pattern.favour); // 修改总计数
                        switch (int idx = (pattern.favour == Player::Black); piece) { // 利用了switch的穿透特性
                            case '_': scores(pattern.favour, pattern.favour)[move] += delta * multiplier * pattern.score;
                            case '^': scores(pattern.favour, -pattern.favour)[move] += delta * multiplier * pattern.score;
                        }
                    };
                    auto update_compound = [&]() {
                        if (Compound::Test(this, move, pattern.favour)) {
                            Compound::Update(this, delta, move, pattern.favour);
                        }
                    };
                    if (delta == 1) {
                        update_pattern(), update_compound();
                    } else {
                        update_compound(), update_pattern();
                    }
                }
            }
        }
    }
}

void Evaluator::updateBlock(int delta, Position move, Player src_player) {
    // 数据准备
    auto sgn = [](int x) { return x < 0 ? -1 : 1; }; // 0以上记为正（认为原位置没有棋）
    auto relu = [](int x) { return std::max(x, 0); };
    auto [density_block, lu, rd] = BlockView(m_density[src_player == Player::Black], move);
    auto [score_block, _, _] = BlockView(m_scores[src_player == Player::Black], move);
    auto [weight_block, score] = BlockWeights;
    // 正式更新
    weight_block = weight_block.block(lu.x(), lu.y(), rd.x() - lu.x() + 1, rd.y() - lu.y() + 1);   
    // 除去旧density分数（初始情况下density_block为0矩阵，减去也没有影响）
    score_block -= score * density_block.unaryExpr(relu);    
    // 更新density（sgn对应原位置是否有棋，delta对应下棋还是悔棋）。
    density_block += density_block.unaryExpr(sgn) * delta * weight_block;
    // 修改中心位置的状态（是否可以用来计分）
    for (auto perspective : { Player::Black, Player::White }) {
        switch (auto& count = m_density[perspective == Player::Black][move]; delta) {
        case 1: // 代表这里下了一子
            count *= -1, count -= 1; break; // 反转计数，表明这个位置已有棋子占用，并减去一辅助数，保证count一定是负数
        case -1: // 代表悔了这一子
            count += 1, count *= -1; break; // 除去之前减掉的辅助数后，反转计数，表明这里可以用来计分了
        }
    }
    // 补回新density分数
    score_block += score * density_block.unaryExpr(relu);
}

void Evaluator::updateMove(Position move, Player src_player) {
    for (auto dir : Directions) {
        auto target = m_boardMap.lineView(move, dir);
        updateLine(target, -1, move, dir); // remove pattern count on original state
    }
    if (src_player != Player::None) {
        m_boardMap.applyMove(move);
        updateBlock(1, move, board().m_curPlayer);
    } else {
        m_boardMap.revertMove();
        updateBlock(-1, move, -board().m_curPlayer | board().m_winner);
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
    for (int i = 0; i < board.m_moveRecord.size(); ++i) {
        if (i < this->board().m_moveRecord.size()) {
            if (this->board().m_moveRecord[i] == board.m_moveRecord[i]) {
                continue;
            } else {
                revertMove(this->board().m_moveRecord.size() - i); // 回退到首次不同步的状态
            }
        }
        applyMove(board.m_moveRecord[i]);
    }
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
}

void Evaluator::Record::set(int delta, Player player) {
    unsigned offset = (player == Player::Black ? 4*sizeof(field) : 0);
    field += delta << offset;
}

void Evaluator::Record::set(int delta, Player favour, Player perspective, Direction dir) {
    unsigned group = (favour == Player::Black) << 1 | (perspective == Player::Black);
    unsigned offset = 4 * group + int(dir);
    field &= ~(1 << offset) | ((delta == 1) << offset);
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
    unsigned offset = (player == Player::Black ? 4*sizeof(field) : 0);
    return (field >> offset) & ((1 << 4*sizeof(field)) - 1);
}

bool Evaluator::Compound::Test(Evaluator * ev, Position pose, Player player) {
    unsigned bit_field = 0; // 下面是个人感觉较妙的算法……
    for (auto comp_type : Components) {
        // bit_field汇总了四个方向的L3/D3/L2设置情况。
        // 由于位或的特性，一条线上同时有LiveTwo与DeadThree时，只会计入一次，规避了BUG。
        bit_field |= ev->m_patternDist[pose][comp_type].get(player, player);
    }
    // is-power-of-2算法，快速判断是否有>=2个bit位为1（>=2两方向有D2或L3）
    return bit_field & (bit_field - 1);
}

void Evaluator::Compound::Update(Evaluator* ev, int delta, Position pose, Player player) {
    // 准备数据
    enum State { S0, L2, LD3, To33, To43, To44 } state = S0; // 自动机的状态定义
    array<Pattern::Type, 4> components; components.fill(Pattern::Type(-1)); // -1代表该方向上没有匹配到有效模式。
    bool restricted = false; // 为true则代表对手想要反制，只能堵在最关键点。
    // 自动机范式编程
    for (auto dir : Directions) {
        // 准备转移条件
        int cond = S0;
        for (auto comp_type : Components) {
            if (ev->m_patternDist[pose][comp_type].get(player, player, dir)) {
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
        if (cond == S0) {
            continue;
        }
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
    auto type = state - State::To33;
    for (int i = 0; i < 4; ++i) {
        if (components[i] == -1) {
            continue;
        }
        auto dir = Direction(i);
        // 更新己方分数与计数
        ev->m_compoundDist[pose][type].set(delta, player, player, dir);
        ev->m_compoundDist.back()[type].set(delta, player); // 增加计数
        ev->scores(player, player)[pose] += delta * 4 * Compound::BaseScore; // 4为奖励因子
        // 更新对方反制分数
        if (restricted) { // 只更新关键点（pose）
            ev->m_compoundDist[pose][type].set(delta, player, -player, dir);
            ev->scores(player, -player)[pose] += delta * 4 * Compound::BaseScore;
        } else { // 更新所有可反制这条线上的棋型的空点。最终关键点分数至少为非关键点的4倍。
            // 为防止溢出，采用试探性自增/减方式更新
            for (int ds : { 1, -1 }) { // ds与dx, dy意思差不多……
                Position current(pose);
                for (int offset = 0; offset < TARGET_LEN / 2; ++offset) {
                    // 即使出错了，在MCTS或AlphaBeta协助下也能很快被软剪枝掉
                    if (ev->m_patternDist[current][components[i]].get(player, -player, dir)) {
                        ev->m_compoundDist[current][type].set(delta, player, -player, dir);
                        ev->scores(player, -player)[current] += delta * Compound::BaseScore; // 无奖励因子
                    }
                    auto [x, y] = current; auto [dx, dy] = *dir;
                    if ((x + dx >= 0 && x + dx < WIDTH) && (y + dy >= 0 && y + dy < HEIGHT)) {
                        current = Shift(current, ds, dir);
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
    { "+xxxxx",   Pattern::Five,      9999 },
    { "-_oooo_",  Pattern::LiveFour,  9000 },
    { "-xoooo_",  Pattern::DeadFour,  2500 },
    { "-o_ooo",   Pattern::DeadFour,  3000 },
    { "-oo_oo",   Pattern::DeadFour,  2600 },
    { "-~_ooo_~", Pattern::LiveThree, 3000 },
    { "-x^ooo_~", Pattern::LiveThree, 2900 },
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
    { "-x_o___x", Pattern::DeadOne,   40 },
    { "-x__o__x", Pattern::DeadOne,   50 },
};

tuple<Array<int, BLOCK_SIZE, BLOCK_SIZE>, int> Evaluator::BlockWeights = []() {
    tuple_element_t<0, decltype(BlockWeights)> weight;
    tuple_element_t<1, decltype(BlockWeights)> score = 15;
    weight<< 2, 0, 0, 2, 0, 0, 2,
             0, 4, 3, 3, 3, 4, 0,
             0, 3, 5, 4, 5, 3, 0,
             1, 3, 4, 0, 4, 3, 1,
             0, 3, 5, 4, 5, 3, 0,
             0, 4, 3, 3, 3, 4, 0,
             2, 0, 0, 2, 0, 0, 2;
    return make_tuple(weight, score);
}();

const Pattern::Type Evaluator::Compound::Components[3] = { 
    Pattern::LiveThree, Pattern::DeadThree, Pattern::LiveTwo 
};
const int Evaluator::Compound::BaseScore = 800;

}