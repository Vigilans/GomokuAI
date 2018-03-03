#include <unordered_map>
#include <vector>


namespace Gomoku {

using namespace std;

static constexpr int width = 19;
static constexpr int height = 19;
static constexpr int max_renju = 5;

class Player {
public:
    enum : __int8 { White = -1, None = 0, Black = 1 } type;

public:
    Player(decltype(type) type) : type(type) {}

    constexpr Player operator-() {  return Player(-this->type); }
    constexpr operator bool() { return this->type == Player::None; }    
};

namespace Test {
    enum class Player : __int8 { White = -1, None = 0, Black = 1 };

    //constexpr operator bool(const Player& player) { return static_cast<bool>(player); }
    constexpr Player operator-(const Player& player) { return Player(-static_cast<int>(player)); }
}


struct Position {
    int id;

    Position(int id) : id(id) { }
    Position(int x, int y) {}

    operator int() const { return id; }

    int x() const { return 0; }
    int y() const { return 0; }

};

/*
    由于这个对象可能会被频繁拷贝，因此拷贝成本需要设计得尽量低。
    棋盘的存储有几种选择：
    1. bitset: 使用bitset存储整个棋盘。由于每个格子需要标记三种落子状态（黑/白/无），至少需要2bit存储。
       因此一个19*19的棋盘需要至少19*19*2=722bit=90.25Byte。
    2. pos -> player的map。一个unordered_map<Position, Player>在MSVC下为>=32Byte。一个{pos, player}对至少为2字节。
       由于采用Hash Table机制， 空间需求很可能更大……
    3. 存储pair<pos, player>的vector。
*/
class Board {
public:

    /*
        返回值类型为Player，代表下一轮应下的玩家。具体解释：
        - 若Player为对手，则代表下一步应对手下，正常继续。
        - 若Player为己方，则代表这一手无效，应该重下。
        - 若Player为None，则代表不用继续下了，下这一步已经获胜。
    */
    Player applyMove(const Position& move) {
        if (checkMove(move)) {
            moves[move] = current;
            current = -current;
            return checkVictory(move) ? Player::None : current;
        } else {
            return current;
        }
    }

private:

    bool checkMove(const Position& move) {
        // 暂无特殊规则
        return true;
    }

    // 每下一步都进行胜利检查，这样便只需对当前落子周围进行遍历即可。
    bool checkVictory(const Position& move) {
        const int curX = move.x(), curY = move.y();
        
        // 沿不同方向的搜索方法复用
        const auto search = [&](auto direction) {
            // 判断坐标的格子是否未越界且归属为当前玩家
            const auto isCurrentPlayer = [&](int x, int y) {
                return (x >= 0 && x < width) && (y >= 0 && y < height) && moves[Position(x, y)] == current;
            };

            int renju = 0; // 当前落子构成的最大连珠数

            // 正向搜索
            int x = curX, y = curY;
            for (int i = 1; i < 4; ++i) {
                direction(x, y);
                if (isCurrentPlayer(x, y)) ++renju;
                else break;
            }

            // 反向搜索
            x = -curX, y = -curY;        
            for (int i = 1; i < 4; ++i) {
                direction(x, y);
                if (isCurrentPlayer(-x, -y)) ++renju;
                else break;
            }

            return renju == 5;
        };
        
        return search([](int& x, int& y) { x += 1; }) // 左->右
            || search([](int& x, int& y) { y += 1; }) // 下->上
            || search([](int& x, int& y) { x += 1, y -= 1; }) // 左上->右下
            || search([](int& x, int& y) { x += 1, y += 1; });// 左下->右上
    }

public:
    
    unordered_map<Position, Player> moves;
    Player current;
};

}