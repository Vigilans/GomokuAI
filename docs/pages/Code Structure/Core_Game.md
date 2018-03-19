### Game.h

#### Player: enum

```cpp
enum class Player : char { White = -1, None = 0, Black = 1 };
```

一个玩家完全可以使用一个整数来存储与表示。

而`enum`类型便是「**纯整数存储+信息封装+概念抽象**」的最好选择。

使用了`enum`之后，我们可以在一个很低的时空开销下抽象出`Player`这么一个事物。`enum class`的表示特征（必须使用`Player::XXX`表示，无法隐式与整数互转）极大地提高了代码的可读性。

同时，利用其内部整数的存储特征，我们可以很巧妙地实现如下的功能：

* 己方→对方的转换。我们实现如下一个函数：

  ```cpp
  constexpr Player operator-(Player player) { 
      return Player(-static_cast<char>(player)); 
  }
  ```

  此后，对于Player对象，只需添加`-`号，便可将其转化为对手玩家。（注意`Player::None`的对立方仍为`Player::None`）

* 游戏结束后，玩家得分的计算：

  ```cpp
  constexpr double getFinalScore(Player player, Player winner) { 
      return static_cast<double>(player) * static_cast<double>(winner); 
  }
  ```

  可以发现，同号（winner与player相同）为正1，异号为负1，平局为零。利用这一巧妙的数学性质，我们避免了条件判断，轻松实现了各方玩家的绝对价值计算。

* 通过`Player`检索棋盘状态：

  ```cpp
  std::array<bool, GameConfig::BOARD_SIZE>& Board::moveStates(Player player) { 
      return m_moveStates[static_cast<int>(player) + 1]; 
  }
  ```

  将[-1, 0, 1]往右偏移1单位，便得到了他们在棋盘上的状态数组。





#### Position: struct

```cpp
struct Position {
    int id;

    Position(int id = -1)         : id(id) {}
    Position(int x, int y)        : id(y*WIDTH + x) {}
    Position(pair<int, int> pose) : id(pose.second*WIDTH + pose.first) {}
    
    int x() const { return id % WIDTH; }
    int y() const { return id / WIDTH; }
    operator int () const { return id; }
};
```

与`Player`一样，`Position`只是一个整数的一层浅浅的封装。在很多情况下，将棋盘表示为一个225格的一维数组要比表示成15*15格的二维数组要更加方便与高效。

为了使它同时看起来像「一个整数」与「一个坐标」，我们为其拓展出了多个(x, y) 与单个id间的相互转换方法。

同时，通过提供`std::pair`作为构造参数，我们常常可以写出 `{x, y}` 这样方便的初始化格式。



#### Board: class

```cpp
class Board {
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
        悔棋。返回值类型为Player，代表下一轮应下的玩家。具体解释：
        - 若Player为对手，则代表悔棋成功，改为对手玩家下棋。
        - 若Player为己方，则代表悔棋失败，棋盘状态不变。
    */
    Player revertMove(Position move);
    
	// 获取随机的一手。即使有禁手规则，也保证返回可下的一手。
    Position getRandomMove() const;

    // 越界与无子检查。暂无禁手规则。
    bool checkMove(Position move);
    
    // 每下一步都进行终局检查，这样便只需对当前落子周围进行遍历即可。
    bool checkGameEnd(Position move);
    
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
        整个数组表示了棋盘上每个点的状态。各下标对应关系为：
        0 - Player::White - 白子放置情况
        1 - Player::None  - 可落子位置情况
        2 - Player::Black - 黑子放置情况
        附：为什么不使用std::vector呢？因为vector<bool> :)。
    */
    std::size_t                    m_moveCounts[3] = {};
    std::array<bool, WIDTH*HEIGHT> m_moveStates[3] = {};
};
```

Board的每一步方法都被仔细地设计过，力求无任何冗余操作。

举例：大多数AI的胜利检查都会遍历整个棋盘，并且每个回合都会检查一次。由于有「每回合检查一次」这个必要的特点，checkGameEnd()函数利用「五子棋每下一子，只需检查当前子周围是否连成五子即可」的推理，使得在O(1)的时间内，就可以完成游戏是否结束的检查。

> 关于Board类是如何被迭代设计到现今的样子的，可参阅*Review*中的[*Improvements*](#Improvements Accumulated)章节。