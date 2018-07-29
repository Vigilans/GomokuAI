#include "ACAutomata.h"
#include <algorithm>
#include <numeric>
#include <queue>

using namespace std;

namespace Gomoku {

AhoCorasickBuilder::AhoCorasickBuilder(initializer_list<Pattern> protos)
    : m_tree{ Node{} }, m_patterns(protos) {

}

void AhoCorasickBuilder::build(PatternSearch* searcher) {
    this->reverseAugment();
    this->flipAugment(); 
    this->boundaryAugment(); 
    this->sortPatterns();
    this->buildNodeBasedTrie();
    this->buildDAT(searcher);
    this->buildACGraph(searcher);
}

void AhoCorasickBuilder::reverseAugment() {
    for (int i = 0, size = m_patterns.size(); i < size; ++i) {
        Pattern reversed(m_patterns[i]);
        std::reverse(reversed.str.begin(), reversed.str.end());
        if (reversed.str != m_patterns[i].str) {
            m_patterns.push_back(std::move(reversed));
        }
    }
}

void AhoCorasickBuilder::flipAugment() {
    for (int i = 0, size = m_patterns.size(); i < size; ++i) {
        Pattern flipped(m_patterns[i]);
        flipped.favour = -flipped.favour;
        for (auto& piece : flipped.str) {
            if (piece == 'x') piece = 'o';
            else if (piece == 'o') piece = 'x';
        }
        m_patterns.push_back(std::move(flipped));
    }
}

void AhoCorasickBuilder::boundaryAugment() {
    for (int i = 0, size = m_patterns.size(); i < size; ++i) {
        char enemy = m_patterns[i].favour == Player::Black ? 'o' : 'x';
        auto first = m_patterns[i].str.find_first_of(enemy);
        auto last = m_patterns[i].str.find_last_of(enemy);
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
}

void AhoCorasickBuilder::sortPatterns() {
    // 根据最终的patterns数量预留空间
    vector<int> codes(m_patterns.size());
    vector<int> indices(m_patterns.size());

    // 编码、填充与排序
    std::transform(m_patterns.begin(), m_patterns.end(), codes.begin(), [](const Pattern& p) {
        auto align_offset = std::pow(std::size(Codeset), MAX_PATTERN_LEN - p.str.size());
        return std::accumulate(p.str.begin(), p.str.end(), 0, [&](int sum, char ch) {
            return sum *= std::size(Codeset), sum += EncodeCharset(ch);
        }) * align_offset; // 按size(Codeset)进制记数并对齐
    }); // 通过对齐的基数排序间接实现字典序排序
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
}

/*
    利用set（红黑树）来做Trie树的基础存储，其内部结点的排序为：
    第一优先级：depth维度
      所有结点通过层次完成第一轮排序。
    第二优先级：siblings维度
      同一层所有结点的[first, last)区间构成了根节点的[0, size)区间的一个划分。
      因此，同一层的结点可按照first排序进行索引。
    由此可见，对于Node来说，其在set的位置可通过{ depth, first }唯一确定，last是可变的。

    由以上定义可知：
      1. 某结点{ depth, first, last }的子结点集合为depth + 1层的[first, last)区间。
      2. 对所有叶结点有last - first == 1。尽管depth不同，但结点相互连接后，便形成了完整的Patterns数组。
*/
void AhoCorasickBuilder::buildNodeBasedTrie() {
    // suffix为以parent的子结点为起点，直至整个pattern结束的后缀
    function<void(string_view, NodeIter)> insert_pattern = [&](string_view suffix, NodeIter parent) {
        if (suffix.empty()) {
            // Base case: 已抵达叶结点，代表记录到一个匹配模式。
            // 确定找到一个模式后，插入哨兵叶结点，并使区间右端点右移一位。
            m_tree.emplace(0, parent->depth + 1, parent->first, ++parent->last);
        } else {
            Node key = { // 准备子结点索引
                EncodeCharset(suffix[0]),
                parent->depth + 1
            };
            auto [first, last] = children(parent);
            auto child = std::find_if(first, last, [&key](const Node& node) { 
                return node.code == key.code; // 仅按前缀字符匹配该结点
            }); 
            if (child == last) { // 前缀匹配失败，新生成一个分支结点
                key.first = parent->last;
                key.last = key.first;
                child = m_tree.insert(key).first;
            }
            insert_pattern(suffix.substr(1), child); // 递归将pattern剩下的部分插入trie树中
            parent->last = child->last; // 反向传播，更新区间右边界值。
        }
    };
    auto root = m_tree.find({});
    for (auto pattern : m_patterns) {
        insert_pattern(pattern.str, root); // 按字典序插入pattern
    }
}

/*
    将set<Node>形式的Trie树压缩至双数组形式。
    构造完毕后，base与check数组各定义为：
    base:
      1. 对于非叶结点，base值为所有子结点的偏移基准值（t = base[s] + c）。
      2. 对于叶结点，base值为结点所对应模式的下标的负数（patterns[-base[l]]）。
      3. 对于空闲位置，base值为空闲链表的前驱结点的下标的负数（v_{i-1} = -base[v_i])。
    check:
      1. 对于一般结点，check值为其父结点所在位置（s = check[t <- base[s] + c]）。
      2. 对于空闲位置，check值为空闲链表的后继结点的下标的负数（v_{i+1} = -check[v_i])。
      3. 对于根节点，由于其无父结点，故没有一般check值，该位置用作空闲链表索引的起点。

    对于以上定义，base值可能有三种情况为0:
      1. 偏移值为0（如根节点）
      2. 对应模式的下标为0（第一个叶结点）
      3. 前驱结点的下标为0（首个空闲位置）
    3与1,2的工作过程没有交集（构建好的自动机在匹配过程中不会落入空闲位置）
    对于1,2，叶结点的判定条件为：check[base[s]] == s
      * 根节点的base[0] == 0，而根据以上定义check[0] < 0，因此不会误判根节点有叶结点。
      * 对应模式的下标为0，则check[0] < 0，也不会判断叶结点还有叶结点。
    因此也不会产生冲突。
*/
void AhoCorasickBuilder::buildDAT(PatternSearch* ps) {
    // index为node在双数组中的索引，之前的递归中已确定好
    function<void(int, NodeIter)> build_recursive = [&](int index, NodeIter node) {
        if (node->depth > 0 && node->code == 0) {
            // Base case: 已抵达叶结点，设置index的base为(-对应pattern表的下标)
            // 此时node的last - first == 1, [first, last)唯一确定了一个结点
            ps->m_base[index] = -node->first; 
        } else {
            // 准备数据
            auto [first, last] = children(node);
            int begin = 0, front = 0;

            /*
                初始条件：begin从0开始（此外根节点base值为0），front沿0往下寻找空位。
                退出条件：找到一个可以容纳index结点的所有子结点的begin值
                状态转移：沿前向链表找到下一个空位，将这个空位视作容纳首个子结点的位置
                参考：http://www.aclweb.org/anthology/D13-1023
            */
            do {
                front = -ps->m_check[front]; // 首个子结点的下标
                begin = front - first->code; // 子结点的偏移基准值

                // 由于负的base值有特殊语义，begin值必须大于0。
                if (begin < 0) continue;

                // 空间不足时扩充m_base与m_check数组。
                // 阈值设为size - 1以保证最后一位为空（下式1移到了左侧以防止溢出）
                while (begin + std::size(Codeset) + 1 >= ps->m_check.size()) {
                    // 扩充空间
                    auto pre_size = ps->m_base.size();
                    ps->m_base.resize(2 * pre_size);
                    ps->m_check.resize(2 * pre_size);
                    // 填充下标补全双链表
                    for (int i = pre_size; i < ps->m_base.size(); ++i) {
                        ps->m_base[i] = -(i - 1); // 逆向链表
                        ps->m_check[i] = -(i + 1); // 前向链表
                    }
                }

            // 验证是否所有候选子结点位置均未被占用
            // 筛选条件：根节点/check值不小于0的结点是被占用的。
            } while (!std::all_of(first, last, [&](const Node& node) {
                auto c_i = begin + node.code;
                return c_i != 0 && ps->m_check[c_i] < 0;
            }));

            // 遍历子结点，设置相关状态后对子结点递归构建
            for (auto cur = first; cur != last; ++cur) {
                // 取得当前子节点下标（值得注意的是叶结点下标就为begin）
                int c_i = begin + cur->code;

                // 将当前下标移出空闲节点链表（利用Dancing Links）
                ps->m_check[-ps->m_base[c_i]] = ps->m_check[c_i];
                ps->m_base[-ps->m_check[c_i]] = ps->m_base[c_i];

                // 将子结点check值与父结点绑定
                ps->m_check[c_i] = index;
            }
            // 父结点的base设置为找好的begin值
            ps->m_base[index] = begin; 
            // Recursive Step: 对每个子结点递归构造
            for (auto cur = first; cur != last; ++cur) {
                build_recursive(begin + cur->code, cur);
            }
        }
    };
    ps->m_base.resize(1, 0);   // 根节点(0)没有前驱结点，故其base位不为逆向链表的标记点，而用作本义base值。
    ps->m_check.resize(1, -1); // 根节点(0)由于没有父结点，故其无本义check值，该位置用来作为前向链表的起点。
    auto root = m_tree.find({});
    build_recursive(0, root);
    ps->m_patterns.swap(m_patterns);
}

void AhoCorasickBuilder::buildACGraph(PatternSearch* ps) {
    // 初始，所有结点的fail指针都指向根节点
    ps->m_fail.resize(ps->m_base.size(), 0);
    ps->m_invariants.resize(std::size(Codeset) + 1, 0);

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

        // 根节点fail指针指向自己，不用设置
        if (cur_node == 0) continue;

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
        // 若某结点接受code后转移至自己，则该节点为「不动点状态」
        if (ps->m_check[ps->m_base[cur_node] + code] != cur_node &&
            ps->m_base[ps->m_fail[cur_node]] + code == cur_node) { 
            ps->m_invariants[code] = cur_node;
        }
    }
}

}
