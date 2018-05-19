#ifndef GOMOKU_HANDICRAFT_MSTREE_H_
#define GOMOKU_HANDICRAFT_MSTREE_H_
#include <algorithm>
#include <memory>

#define CACHE_LINE 64

namespace Gomoku::Handicrafts {

// Cache-Friendly Multiway Search Tree
// Key type should provide operator bool to confirm stop flag.
template <typename Key, typename Value, 
    size_t Ways = CACHE_LINE / sizeof(Key)>
class MSTree {
public:
    struct Node {
        Node* parent = nullptr; // 观察指针
        Key keys[Ways] = {}; // 索引域
        Value values[Ways] = {}; // 数据域
        std::unique_ptr<Node> childs[Ways]; // 规定末尾元素为最大值，因此是M元素M路。
    };

public:
    template <typename RandomAccessIter, typename Converter>
    MSTree(RandomAccessIter beg, RandomAccessIter end, Converter keyOf)
        : m_root(init_recursive(beg, end, keyOf)) { }

    Value* search(Key key) {
        for (Node* node = m_root.get(); node != nullptr;) {
            for (int i = 0; i <= Ways; ++i) {
                if (!node->keys[i] || i == Ways) {
                    node = node->parent;
                    break;
                }
                if (key < node->keys[i]) {
                    node = node->childs[i].get();
                    break;
                }
                if (key == node->keys[i]) {
                    return &node->values[i];
                }
            }  
        }
        return nullptr;
    }

private:
    // A sorted sequence must be passed.
    template <typename RandomAccessIter, typename Converter>
    std::unique_ptr<Node> init_recursive(RandomAccessIter beg, RandomAccessIter end, Converter keyOf) {
        if (beg >= end) {
            return nullptr;
        }
        auto node = new Node;
        auto length = end - beg;
        auto interval = (length - 1) / Ways;
        for (int i = 0; i < Ways; ++i) {
            auto iBeg = beg + i * (interval + 1);
            auto iEnd = iBeg + std::min<int>(interval, end - iBeg - 1);
            node->keys[i] = keyOf(*iEnd);
            node->values[i] = *iEnd;
            node->childs[i] = init_recursive(iBeg, iEnd, keyOf);
            if (iEnd == end - 1) {
                break;
            }
        }
        return std::unique_ptr<Node>(node);
    }

    std::unique_ptr<Node> m_root;
};

}

#endif // !GOMOKU_HANDICRAFT_MSTREE_H_
