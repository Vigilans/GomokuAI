#pragma once
#include <string>
#include <vector>
#include <algorithm>
#include <unordered_map>
#include <unordered_set>
#include <cmath>

namespace Gomoku {

using namespace std; // FIXME：坏习惯，在未来将被迁移到cpp中

static constexpr int width = 15;
static constexpr int height = 15;
static constexpr int max_renju = 5;

enum class Player : char { White = -1, None = 0, Black = 1 };

constexpr Player operator-(Player player) {
    // 返回对手Player值；Player::None的对手仍是Player::None
    return Player(-static_cast<char>(player));
}

constexpr float getFinalScore(Player player, Player winner) {
    // 同号（winner与player相同）为正，异号为负，平局为零
    return static_cast<float>(player) * static_cast<float>(winner);
}

// 可用来表示第一/第四象限的坐标。也就是说，x/y坐标必须同正或同负。
struct Position {
    int id;

    Position(int id = -1) : id(id) { }
    Position(int x, int y) : id(y * width + x) {}
    Position(std::pair<int, int> pose) : id(pose.second * width + pose.first) {}

    operator int() const { return id; }

    constexpr int x() const { return id % width; }
    constexpr int y() const { return id / width; }

    // 保证Position可以作为哈希表的key使用
    struct Hasher {
        std::size_t operator()(const Position& position) const {
            return position.id;
        }
    };
};


/*
    由于这个对象可能会被频繁拷贝，因此拷贝成本需要设计得尽量低。
    棋盘的存储有几种选择：
    1. bitset: 使用bitset存储整个棋盘。由于每个格子需要标记三种落子状态（黑/白/无），至少需要2bit存储。
       因此一个19*19的棋盘需要至少19*19*2=722bit=90.25Byte。
    2. pos -> player的map。一个unordered_map<Position, Player>在MSVC下为>=32Byte。一个{pos, player}对至少为3字节。
       由于采用Hash Table机制， 空间需求很可能更大……
    3. 存储pair<pos, player>的vector。
*/
class Board {
public:
    static Board load(wstring boardPath) {
        
    }

public:
    Board() : 
        m_curPlayer(Player::Black), 
        m_winner(Player::None),
        m_appliedMoves(width * height),
        m_availableMoves(width * height) {
        for (int i = 0; i < width * height; ++i) {
            m_availableMoves.emplace(i);
        }
    }

    /*
        返回值类型为Player，代表下一轮应下的玩家。具体解释：
        - 若Player为对手，则代表下一步应对手下，正常继续。
        - 若Player为己方，则代表这一手无效，应该重下。
        - 若Player为None，则代表这一步后游戏已结束。此时，可以通过m_winner获取游戏结果。
    */
    Player applyMove(Position move) {
        // 检查游戏是否未结束且为有效的一步
        if (m_curPlayer != Player::None && checkMove(move)) {
            m_appliedMoves[move] = m_curPlayer;
            m_availableMoves.erase(move);
            m_curPlayer = checkVictory(move) ? Player::None : -m_curPlayer;
        }
        return m_curPlayer;
    }

    Player revertMove(Position move) {
        if (m_appliedMoves.count(move) && m_appliedMoves[move] != m_curPlayer) {
            m_curPlayer = m_appliedMoves[move];
            m_availableMoves.insert(move);
            m_appliedMoves.erase(move);
        }
        return m_curPlayer;
    }

    const auto status() const {
        struct { bool end; Player winner; } status { 
            m_curPlayer == Player::None, 
            m_winner 
        };
        return status;
    }

    // a hack way to get a random move
    // referto: https://stackoverflow.com/questions/12761315/random-element-from-unordered-set-in-o1/31522686#31522686
    Position getRandomMove() {
        if (m_availableMoves.empty()) {
            throw overflow_error("board is already full");
        }
        //int divisor = (RAND_MAX + 1) / (width * height);
        //auto rnd = Position(rand() % divisor);
        //while (!m_availableMoves.count(rnd)) {
            //rnd.id = rand() % divisor;

            //rnd.id = (rnd.id + 1) % (width * height);

            //board.m_availableMoves.insert(rnd);
            //auto iter = board.m_availableMoves.find(rnd);
            //rnd = *(next(iter) == board.m_availableMoves.end() ? board.m_availableMoves.begin() : next(iter));
            //board.m_availableMoves.erase(iter);
        //}
        // referto: https://stackoverflow.com/questions/27024269/
        int bucket = rand() % m_availableMoves.bucket_count();
        int bucketSize;
        while ((bucketSize = m_availableMoves.bucket_size(bucket)) == 0) {
            bucket = (bucket + 1) % m_availableMoves.bucket_count();
        } 
        auto rnd = *std::next(m_availableMoves.begin(bucket), rand() % bucketSize);
        return rnd;
    }

private:
    bool checkMove(Position move) {    
        #if _DEBUG
        // 若在调试模式下，则加一层是否越界且是否有子的检查
        return move.id >= 0 && move.id < width * height && !m_appliedMoves.count(move);
        #else
        // 暂无禁手规则
        return true;
        #endif
    }

    // 每下一步都进行终局检查，这样便只需对当前落子周围进行遍历即可。
    bool checkVictory(Position move) {
        const int curX = move.x(), curY = move.y();
        
        // 沿不同方向的搜索方法复用
        const auto search = [&](int dx, int dy) {
            // 判断坐标的格子是否未越界且归属为当前棋子
            const auto isCurrentPlayer = [&](int x, int y) {
                Position pose(x, y);
                return (x >= 0 && x < width) && (y >= 0 && y < height) 
                    && (m_appliedMoves.count(pose) && m_appliedMoves[pose] == m_curPlayer);
            };

            int renju = 1; // 当前落子构成的最大连珠数

            // 正向与反向搜索
            for (int sgn = 1; sgn != -1; sgn = -1) {
                int x = curX, y = curY;
                for (int i = 1; i < 4; ++i) {
                    x += sgn*dx, y += sgn*dy;
                    if (isCurrentPlayer(x, y)) ++renju;
                    else break;
                }
            }

            return renju >= 5;
        };
        
        // 从 左->右 || 下->上 || 左上->右下 || 左下->右上 进行搜索
        if (search(1, 0) || search(0, 1) || search(1, -1) || search(1, 1)) {
            m_winner = m_curPlayer; // 赢家为当前玩家
            return true;
        } else if (m_availableMoves.empty()) {
            m_winner = Player::None; // 若未赢，之后也没有可下之地，则为和局
            return true;
        } else {
            return false;
        }
    }

public:
    // 当值为Player::None时，代表游戏结束。
    Player m_curPlayer;

    /*
        当棋局结束后，其值代表最终游戏结果:
        - Player::Black: 黑赢
        - Player::White: 白赢
        - Player::None:  和局
        当棋局还未结束时（m_curPlayer != Player::None），值始终保持为None，代表还没有赢家。
    */
    Player m_winner;

    // 所有已落子位置。存储 { 位置，落子颜色 } 的键值对。
    unordered_map<Position, Player, Position::Hasher> m_appliedMoves;

    // 所有可落子位置。由于值均为Player::None，故用set存储即可。
    unordered_set<Position, Position::Hasher> m_availableMoves;
};

}