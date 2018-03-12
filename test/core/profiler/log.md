## 2018/3/11
### DEBUG下性能瓶颈：
* playout() -> 92.01%
  * simulate() -> 71.14%
    * getRandomMove() -> 40.65%
	* applyMove() -> 28.26%
  * reset() -> 18.69%
  * expand() -> 0.18%

* 总计：applyMove() -> 34.48%
  * checkMove()（DEBUG版） -> 2.18%
  * hashset[xxx] = xxx -> 6.90%
  * **hashset.erase() -> 11.43%**
  * **checkVictory() -> 13.79%**
    * 几乎全部集中在以下代码中：
	  ```cpp
		(x >= 0 && x < width) && (y >= 0 && y < height) 
        && (m_appliedMoves.count(pose) && m_appliedMoves[pose] == m_curPlayer);
	  ```

* 总计：getRandomMove() -> 41.38%
  * **hashset.bucket_size() -> 36.30%**
  * bucket = (bucket + 1) % hashset.bucket_count() -> 2.54%
  * hashset.begin(bucket), rand () % bucketSize -> 1.63


**总结**：DEBUG模式下，STL的开销成为了绝对的大头。



## 2018/3/12

### 将棋盘容器改为std::array，并修改一些BUG后，RELEASE下的性能分析：

相对于`2018/3/11`版本，性能提升了约50倍。
在$10^6$次playout下，一次测试耗时约4秒。

为了获取更详细的函数工作开销，我们关闭了编译时内联扩展，再进行了一次测试：
尽量不考虑一些明显因禁止内联扩展而使得开销增大的函数，目前的CPU开销分布为：

* playout() -> 98.45%
* * applyMove() -> 50.50%
  * reset() -> 23.18%
  * select() -> 10.47% （禁止内联展开引起的开销）
* 总计：applyMove() -> 50.61%
  * checkVictory() -> 39.58%
    * 与`2018/3/11`版本一样，开销仍旧集中在isCurrentPlayer()函数中。
  * setState() -> 2.78%
  * unsetState() -> 1.82%
  * checkMove() -> 1.19%
* 总计：reset() -> 23.18%
  * deque::back() -> 13.02%
  * revertMove() -> 7.21%
    * setState() -> 2.53%
    * unsetState() -> 2.14%


**总结**：

1. 更换std::array后，尽管性能得到了质的提升，但applyMove()与revertMove()仍旧是两个大头。

2. 值得注意的是，在目前的实现中，checkMove()与isCurrentPlayer()所做的事几乎一模一样，但其开销总占比却差了33倍之多。

   可能的原因是：在每次applyMove()中，checkMove()与checkVictory()各被调用一次，但checkVictory会检查4个方向，每个方向至多会检查2*4个子，也就是isCurrentPlayer()最高会被调用32次，造成了巨大的开销。

3. 另外，用于取栈顶元素的deque::back()也值得注意。尽管禁止了内联展开，但其开销还是显得过大了。


**可能的性能建议**：

1. 由于目前版本下isCurrentPlayer()只在一处代码被调用，因此可以将其从函数封装中拆出来。
2. 进一步优化胜利检查的算法，减少isCurrentPlayer()等操作的频繁嗲用
3. 将std::stack的Container改为std::vector，或干脆就将std::vector作为栈使用。



### 对MCTS的playout机制进一步调优后，RELEASE下性能分析：

在$10^6$次playout下，耗时约950ms。相对于上一个版本，性能提升了4倍。
也就是说，相对于昨天的版本，性能整体提升了200倍。

未禁止内联扩展时，CPU开销分布情况：

* playout() -> 99.75%
  * backPropogate() -> 43.18%
    * 里面包含了revertMove()
  * applyMove() ->  39.43%
  * checkGameEnd() -> 7.86%
  * select() -> 7.44%
  * Node::isLeaf() -> 1.44%

禁止内联扩展后，经过Profiler测试，发现applyMove()与revertMove()的大头都明显是由无内联优化而造成的。
这就是说，就目前而言纯随机下法优化能带来的提升暂时不大了。