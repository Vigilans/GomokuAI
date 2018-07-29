#ifndef GOMOKU_AHO_CORASICK_H_
#define GOMOKU_AHO_CORASICK_H_
#include "../include/Pattern.h"
#include <set>

namespace Gomoku {

class AhoCorasickBuilder {
public:
    // 用于构建双数组的临时结点
    struct Node {
        int code, depth, first; mutable int last; // last不参与比较，故可声明为mutable。
        
        // 默认构造时生成根结点(0, 0, 0, 0)
        Node(int code = 0, int depth = 0, int first = 0, int last = -1)
            : code(code), depth(depth), first(first), last(last == -1 ? first: last) { }

        // 根据位编码字符创建结点
        Node(char ch, int depth = 0, int first = 0, int last = -1)
            : code(EncodeCharset(ch)), depth(depth), first(first), last(last == -1 ? first: last) { }
        
        bool operator<(const Node& other) const { // 采用depth->first字典序比较。
            return std::tie(depth, first) < std::tie(other.depth, other.first);
        }

        bool operator==(const Node& other) const {
            return std::tie(code, depth, first, last) == std::tie(other.code, other.depth, other.first, other.last);
        }
    };

    using NodeIter = std::set<Node>::iterator;

public:
    AhoCorasickBuilder(std::initializer_list<Pattern> protos);

    void build(PatternSearch* searcher);

public:
    // 不对称的pattern反过来看与原pattern等价
    void reverseAugment();

    // pattern的黑白颜色对调后与原pattern等价
    void flipAugment();

    // 被边界堵住的pattern与被对手堵住一角的pattern等价
    void boundaryAugment();

    // 根据编码字典序排序patterns
    void sortPatterns();

    // DFS插入生成基于结点的Trie树
    void buildNodeBasedTrie();

    // DFS遍历生成双数组Trie树
    void buildDAT(PatternSearch* ps);

    // BFS遍历，为DAT构建AC自动机的fail指针数组
    void buildACGraph(PatternSearch* ps);

private:
    std::pair<NodeIter, NodeIter> children(NodeIter node) {
        auto first = m_tree.lower_bound({ 0, node->depth + 1, node->first }); // 子节点下界（no less than）
        auto last = m_tree.upper_bound({ 0, node->depth + 1, node->last - 1 }); // 子节点上界（greater than）
        return { first, last };
    }

private:
    std::set<Node> m_tree; // 利用在插入中保持有序的RB树，作为临时保存Trie树的结构
    std::vector<Pattern> m_patterns;
};

}
#endif // !GOMOKU_AHO_CORASICK_H_
