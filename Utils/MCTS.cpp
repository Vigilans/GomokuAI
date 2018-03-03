#include <vector>
#include <algorithm>

namespace Gomoku {

using namespace std;

class MCTS {

public:
    struct Node {
        Node* parent;
        vector<Node*> children;
        size_t visits;


        float UCB1(float expl_param) {
            
        }
    };


public:
    void select() {

    }

private:
    Node * root;

};

}