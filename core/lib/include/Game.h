#ifndef GOMOKU_GAME_H_
#define GOMOKU_GAME_H_
#include <utility> // std::pair, std::size_t
#include <vector>  // std::vector
#include <array>   // std::array
#include <Eigen/Dense> // Eigen::VectorXf

namespace Gomoku {

inline namespace Config {
// 游戏的基本配置
enum GameConfig {
    WIDTH = 15, HEIGHT = 15, MAX_RENJU = 5, 
    BOARD_SIZE = WIDTH*HEIGHT
};
}

// 玩家概念的抽象封装
enum class Player : short { 
    White = -1, None = 0, Black = 1 
};

// 返回对手的Player值。Player::None的对手仍是Player::None。
constexpr Player operator-(Player player) { 
    return Player(-static_cast<short>(player)); 
}

// 从左往右求值，返回首个非Player::None的一方。
constexpr Player operator|(Player lhs, Player rhs) {
    return lhs != Player::None ? lhs : rhs;
}

// 返回游戏结束后的得分。同号（winner与player相同）为正，异号为负，平局为零。
constexpr float CalcScore(Player player, Player winner) { 
    return static_cast<float>(player) * static_cast<float>(winner); 
}

// 返回当前局面评分相对于参数的player的价值。
constexpr float CalcScore(Player player, float stateValue) {
    return static_cast<float>(player) * stateValue;
}


// 可用来表示第一/第四象限的坐标。也就是说，x/y坐标必须同正或同负。
struct Position {
	static const Position npos; // 用于表示无位置的标志量
    short id; // 内部存储使用short，其余情况均转换为int计算。

    constexpr Position(int id = npos)              : id(id) {}
    constexpr Position(int x, int y)             : id(y * WIDTH + x) {}
    constexpr Position(std::pair<int, int> pose) : id(pose.second * WIDTH + pose.first) {}

    constexpr operator int () const { return id; }
    constexpr int x() const { return id % WIDTH; }
    constexpr int y() const { return id / WIDTH; }
};


class Board {
// 公开接口部分
public:
    Board();

    /*
        下棋。返回值类型为Player，代表下一轮应下的玩家。具体解释：
        - 若Player为对手，则代表下一步应对手下，正常继续。
        - 若Player为己方，则代表这一手无效，应该重下。
        - 若Player为None，则代表这一步后游戏已结束。此时，可以通过m_winner获取游戏结果。
    */
    Player applyMove(Position move, bool checkVictory = true);

    /*
        悔棋。具体解释：
        - 参数为要悔棋的步数，默认为1。
        - 返回值类型为Player，代表下一轮应下的玩家。
    */
    Player revertMove(size_t count = 1);

    // 获取均匀概率分布的随机的可行落点。
    Position getRandomMove() const;

    // 根据提供的概率分布随机获取可行落点。若非法点概率不为0，则不保证返回的点一定合法。
    Position getRandomMove(Eigen::Ref<Eigen::VectorXf> probs) const;

    // 越界与无子检查。暂无禁手规则。
    bool checkMove(Position move) const;

    // 安全版本的越界检查。
    bool checkBoundary(int x, int y) const;

    // 从性能角度考虑，只需对最后落子周围进行遍历。
    bool checkGameEnd();

    // 重置棋盘到初始状态。
    void reset();
    
// 数据成员封装部分
public: 
    // 返回当前游戏状态
    const auto status() const noexcept {
        struct { bool end; Player curPlayer; Player winner; } status {
            m_curPlayer == Player::None, m_curPlayer, m_winner 
        };
        return status;
    }

    // 通过Player枚举获取对应棋盘状态
    std::array<bool, BOARD_SIZE>&       moveStates(Player player) { return m_moveStates[static_cast<int>(player) + 1]; }
    const std::array<bool, BOARD_SIZE>& moveStates(Player player) const { return m_moveStates[static_cast<int>(player) + 1]; }
    
    // 获取棋盘在对应Position上的Player状态。仅提供只读接口。
    bool moveState(Player player, Position pose) const { return m_moveStates[static_cast<int>(player) + 1][pose.id]; }

    // 通过Player枚举获取已落子/未落子总数
    std::size_t& moveCounts(Player player) { return m_moveCounts[static_cast<int>(player) + 1]; }
    std::size_t  moveCounts(Player player) const { return m_moveCounts[static_cast<int>(player) + 1]; }

    // 输出对阅读友好的字符串
    std::string toString() const;

// 数据成员部分
public:
    /*
        表示在当前棋盘状态下应下的玩家。
        当值为Player::None时，代表游戏结束。
        默认为黑棋先下。
    */
    Player m_curPlayer = Player::Black;

    /*
        当棋局结束后，其值代表最终游戏结果:
        - Player::Black: 黑赢
        - Player::White: 白赢
        - Player::None:  和局
        当棋局还未结束时（m_curPlayer != Player::None），值始终保持为None，代表还没有赢家。
    */
    Player m_winner = Player::None;

    /*
        两个数组表示了棋盘上的状态与已落子个数。各下标对应关系为：
        0 - Player::White - 白子放置情况
        1 - Player::None  - 可落子位置情况
        2 - Player::Black - 黑子放置情况
        附：为什么不使用std::vector呢？因为vector<bool> :)。
    */
    std::array<bool, BOARD_SIZE> m_moveStates[3] = {};
    std::size_t m_moveCounts[3] = {};

    //保存了棋局的完整记录的栈式结构。
    std::vector<Position> m_moveRecord;
};

// 统一的随机数引擎


}

// Generic bindings in namespace std
namespace std {

string to_string(Gomoku::Player);
string to_string(Gomoku::Position);
string to_string(const Gomoku::Board&);

template <>
struct hash<Gomoku::Position> {
    std::size_t operator()(const Gomoku::Position& position) const {
        return position.id;
    }
};

// Custom Structure Binding
template <std::size_t N>
constexpr int get(const Gomoku::Position& pose) { return N == 0 ? pose.x() : pose.y(); }

template <>
struct tuple_size<Gomoku::Position> : std::integral_constant<std::size_t, 2> {};

template <std::size_t N>
struct tuple_element<N, Gomoku::Position> { using type = int; };

}

#endif // !GOMOKU_GAME_H_
