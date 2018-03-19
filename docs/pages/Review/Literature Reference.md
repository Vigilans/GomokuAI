## Literatures Reference

### Already In Use

- [x] \[1\][*PENG Bo*: 从围棋盘看卷积神经网络CNN的具体工作过程](https://zhuanlan.zhihu.com/p/25345778)

      虽然目前没有用上CNN，研究对象也不是围棋，但文中提出的「One-hot编码」对我们组的棋盘设计有着很大的启发。采用这种形式构造的棋盘，带来了许多意料之外的好处（尤其是速度）。

### Work-In-Progress

- [ ] [*Browne et al*: A Survey of Monte Carlo Tree Search Methods](http://mcts.ai/pubs/mcts-survey-master.pdf)

      关于MCTS方法的大宗型调研论文。涵盖了从select到backPropogate的MCTS所有子过程的理解、优化与应用。是我们组未来对AI进行优化时最为重要的参考大纲。

### Candidates For Future

- [ ] [*Gelly et al*: Monte-Carlo tree search and rapid action value estimation in computer Go](http://www.ics.uci.edu/~dechter/courses/ics-295/winter-2018/papers/mcts-gelly-silver.pdf)

      RAVE算法。这是作者在2011年发布的版本。整个算法似乎利用到了「重用子问题」的思想，是非常值得研究的一种优化策略。

- [ ] [*Frydenberg et al*: Investigating MCTS Modiﬁcations in General Video Game Playing]()

- [ ] [*Lanzi et al*: Evolving UCT Alternatives for General Video Game Playing](https://www.politesi.polimi.it/bitstream/10589/133902/3/tesi.pdf)

      以上两篇论文都提出了一些UCB1的修改算法。若能从中找到一个较好的无需父结点数据的公式，则可以对MCTS的select过程实现一个很重要的性能优化：通过维护一个大根堆来快速选出最优子结点。