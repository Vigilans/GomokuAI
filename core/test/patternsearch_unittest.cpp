#include "pch.h"
#include <string>
#define private public
#include "lib/include/Pattern.h"
#include "lib/src/utils/ACAutomata.h"
#undef private

namespace Gomoku {
inline bool operator==(const Pattern& lhs, const Pattern& rhs) {
    return std::tie(lhs.str, lhs.type, lhs.favour, lhs.score) == std::tie(rhs.str, rhs.type, rhs.favour, rhs.score);
}
}

using namespace Gomoku;
using namespace std;

inline string operator""_v(const char* str, size_t size) {
    string code(size, 0);
    std::transform(str, str + size, code.begin(), EncodeCharset);
    return code;
}

class PatternSearchTest : public ::testing::Test {
protected:
    AhoCorasickBuilder builder = {
        { "-~_ooo_~", Pattern::LiveThree, 0 },
        { "-x^ooo_~", Pattern::LiveThree, 0 },
        { "-x_ooo_x", Pattern::DeadThree, 1 },
    };

    PatternSearch ps;

    static auto Encode(string_view pattern) {
        vector<int> code(pattern.size());
        std::transform(pattern.begin(), pattern.end(), code.begin(), EncodeCharset);
        return code;
    };
};

TEST_F(PatternSearchTest, AugmentPattern) {
    vector<Pattern> patterns{
        { "-~_ooo_~", Pattern::LiveThree, 0 },
        { "-x^ooo_~", Pattern::LiveThree, 0 },
        { "-x_ooo_x", Pattern::DeadThree, 1 },
    };
    builder.reverseAugment();
    patterns.insert(patterns.end(), {
        { "-~_ooo^x", Pattern::LiveThree, 0 },
    });
    EXPECT_EQ(builder.m_patterns, patterns);
    builder.flipAugment();
    patterns.insert(patterns.end(), {
        { "+~_xxx_~", Pattern::LiveThree, 0 },
        { "+o^xxx_~", Pattern::LiveThree, 0 },
        { "+o_xxx_o", Pattern::DeadThree, 1 },
        { "+~_xxx^o", Pattern::LiveThree, 0 },
    });
    EXPECT_EQ(builder.m_patterns, patterns);
    builder.boundaryAugment();
    patterns.insert(patterns.end(), {
        {"-?^ooo_~", Pattern::LiveThree, 0},
        {"-?_ooo_x", Pattern::DeadThree, 1},
        {"-?_ooo_?", Pattern::DeadThree, 1},
        {"-x_ooo_?", Pattern::DeadThree, 1},
        {"-~_ooo^?", Pattern::LiveThree, 0},
        {"+?^xxx_~", Pattern::LiveThree, 0},
        {"+?_xxx_o", Pattern::DeadThree, 1},
        {"+?_xxx_?", Pattern::DeadThree, 1},
        {"+o_xxx_?", Pattern::DeadThree, 1},
        {"+~_xxx^?", Pattern::LiveThree, 0},
    });
    EXPECT_EQ(builder.m_patterns, patterns);
}

TEST_F(PatternSearchTest, SortPatterns) {
    builder.reverseAugment();
    builder.flipAugment();
    builder.boundaryAugment();
    auto patterns = builder.m_patterns; // 作一份拷贝，分别排序
    builder.sortPatterns();
    // 把pattern编码至Codeset数组
    std::sort(patterns.begin(), patterns.end(), [](const auto& lhs, const auto& rhs) {
        return Encode(lhs.str) < Encode(rhs.str);
    });
    // 按基数排序的效果应该和按字典序排序的效果一样
    EXPECT_EQ(builder.m_patterns, patterns);
}

TEST_F(PatternSearchTest, NodeBasedTrie) {
    builder.reverseAugment();
    builder.flipAugment();
    builder.boundaryAugment();
    builder.sortPatterns();
    builder.buildNodeBasedTrie();
    // 根节点包含的模式范围一定为[0, patterns.size())
    EXPECT_EQ(builder.m_tree.find({})->last, builder.m_patterns.size());
    // 按照预设的builder手动分析插入节点
    decltype(builder.m_tree) tree;
    // builder中给的Pattern构成的Trie树性质实在是太好了……
    tree.insert({ // 根结点
        { 0, 0, 0, 18 }, 
    });
    tree.insert({ // 第一层
        { 'x', 1, 0, 3 },{ 'o', 1, 3, 6 },{ '?', 1, 6, 12 },{ '-', 1, 12, 18 } 
    });
    tree.insert({ // 第二层
        { '-', 2, 0, 3 },{ '-', 2, 3, 6 },{ '-', 2, 6, 12 },{ '-', 2, 12, 18 }
    });
    for (int depth = 3; depth < 6; ++depth) {
        tree.insert({ // 第三~五层
            { 'o', depth, 0, 3 },
            { 'x', depth, 3, 6 },
            { 'x', depth, 6, 9 },  { 'o', depth, 9, 12 },
            { 'x', depth, 12, 15 },{ 'o', depth, 15, 18 },
        });
    }
    for (int first = 0; first < 18; first += 3) {
        tree.emplace('-', 6, first, first + 3); // 第六层
    }
    for (int i = 0; i < 18; ++i) {
        char ch = 0;
        switch (i % 3) { 
            case 0:
                if (i < 6) ch = (i / 3) % 2 ? 'o' : 'x';
                else       ch = (i / 3) % 2 ? 'x' : 'o';
                break;
            case 1: ch = '?'; break;
            case 2: ch = '-'; break;
        }
        tree.emplace(ch, 7, i, i + 1); // 第七层 + 叶结点
        tree.emplace(0, 8, i, i + 1);
    }
    EXPECT_EQ(builder.m_tree, tree); // 检测自动构造与手动构造的树是否相等
}

TEST_F(PatternSearchTest, DoubleArrayTrie) {
    builder.reverseAugment();
    builder.flipAugment();
    builder.boundaryAugment();
    builder.sortPatterns();
    builder.buildNodeBasedTrie();
    builder.buildDAT(&ps);
    // 通过Trie树的前缀匹配进行测试
    string_view positives[] = {
        "x_ooo_x", "?_ooo_x", "?_xxx_?", "_~xxx^o"
    };
    string_view negatives[] = {
        "xoooo_o", "x_oxo_x", "?_oooox", "x_oo"
    };
    const auto match = [&](string_view target) {
        int state = 0;
        while (ps.m_check[ps.m_base[state]] != state && !target.empty()) {
            int next = ps.m_base[state] + EncodeCharset(target[0]);
            if (ps.m_check[next] != state) {
                return false;
            }
            state = next; // 此时state已检查过了，是父结点
            target.remove_prefix(1);
        }
        // 匹配到叶结点则算匹配成功
        return ps.m_check[ps.m_base[state]] == state; 
    };
    for (auto target : positives) {
        EXPECT_TRUE(match(target)) << "Positive sample failed: " << target;
    }
    for (auto target : negatives) {
        EXPECT_FALSE(match(target)) << "Negative sample failed: " << target;
    }
}

TEST_F(PatternSearchTest, ACFailPointers) {
    builder.reverseAugment();
    builder.flipAugment();
    builder.boundaryAugment();
    builder.sortPatterns();
    builder.buildNodeBasedTrie();
    builder.buildDAT(&ps);
    builder.buildACGraph(&ps);
    // 通过手动测试一些fail指针是否指向正确来测试
    const auto travel = [&](string_view target) {
        // 返回一个目标最终匹配到的结点状态
        int state = 0;
        while (ps.m_check[ps.m_base[state]] != state && !target.empty()) {
            int next = ps.m_base[state] + EncodeCharset(target[0]);
            if (ps.m_check[next] != state) {
                break;
            }
            state = next;
            target.remove_prefix(1);
        }
        return state;
    };
    EXPECT_EQ(travel(""), ps.m_fail[travel("")]);
    EXPECT_EQ(travel(""), ps.m_fail[travel("o")]);
    EXPECT_EQ(travel("-"), ps.m_fail[travel("o_")]);
    EXPECT_EQ(travel("x"), ps.m_fail[travel("o_x")]);
    EXPECT_EQ(travel("x"), ps.m_fail[travel("o_xx")]);
    EXPECT_EQ(travel("x"), ps.m_fail[travel("o_xxx")]);
    EXPECT_EQ(travel("x-"), ps.m_fail[travel("o_xxx_")]);
    EXPECT_EQ(travel("x-o"), ps.m_fail[travel("o_xxx_o")]);
}


TEST_F(PatternSearchTest, PatternMatch) {
    builder.build(&ps);
    auto target = "??-xxx-ooo-xxx-o-xxx--xxx-?"_v;
    auto generator = ps.execute(target);
    auto current = generator.begin();
    auto test = [&](string expect_str, int expect_offset) {
        auto [pattern, offset] = *current; ++current;
        if (Encode(pattern.str) == Encode(expect_str) && offset == expect_offset) {
            return ::testing::AssertionSuccess();
        } else {
            return ::testing::AssertionFailure();
        }
    };
    EXPECT_TRUE(test("?-xxx-o", 7));
    EXPECT_TRUE(test("x-ooo-x", 11));
    EXPECT_TRUE(test("o-xxx-o", 15));
    EXPECT_TRUE(test("o-xxx--", 21));
    EXPECT_TRUE(test("--xxx-?", 26));
    EXPECT_FALSE(current != generator.end());
}

TEST_F(PatternSearchTest, InvariantState) {
    auto& ps = Evaluator::Patterns;
    int state, code;
    // 'x'的不动点状态为"xxxxx"
    state = 0, code = EncodeCharset('x');
    for (int i = 0; i < 5; ++i) {
        state = ps.m_base[state] + code;
    }
    EXPECT_TRUE(state == ps.m_invariants[code]);
    // 'o'的不动点状态为"ooooo"
    state = 0, code = EncodeCharset('o');
    for (int i = 0; i < 5; ++i) {
        state = ps.m_base[state] + code;
    }
    EXPECT_TRUE(state == ps.m_invariants[code]);
    // '?'的不动点状态为"?"
    state = 0, code = EncodeCharset('?');
    for (int i = 0; i < 1; ++i) {
        state = ps.m_base[state] + code;
    }
    EXPECT_TRUE(state == ps.m_invariants[code]);
    // '-'的不动点状态为"----"
    state = 0, code = EncodeCharset('-');
    for (int i = 0; i < 4; ++i) {
        state = ps.m_base[state] + code;
    }
    EXPECT_TRUE(state == ps.m_invariants[code]);
}