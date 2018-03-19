### MCTS.h

#### MCTS: class

```cpp
struct Node {     
    Node* parent = nullptr; // 子结点仅对父结点持有观察指针，不拥有所有权
    vector<unique_ptr<Node>> children = {};
    Position position = -1;
    size_t visits = 0;
    float value = 0.0;

    bool isFull(const Board& board) const;
    float UCB1(float expl_param);
};

class MCTS {
public:
    MCTS(Player player, Position lastMove = -1);
	
    // 利用MCTS预测下一手
    Position getNextMove(Board& board); 

private:
    // 蒙特卡洛树的一轮迭代
    void playout(Board& board);
    
    // 对MCTS的再利用策略
    void stepForward();

private:
    Node* select(const Node* node) const;
    Node* expand(Node* node, Board& board);
    float simulate(Node* node, Board& board);
    void backPropogate(Node* node, Board& board, float value);

private:
    unique_ptr<Node> m_root;
    Player m_player;
    vector<Position> m_moveStack;
};
```

截止报告提交时，MCTS的结构有了很大的改变。

不过，这些改变和MCTS结构的具体介绍，就留给第6周的报告来总结吧！