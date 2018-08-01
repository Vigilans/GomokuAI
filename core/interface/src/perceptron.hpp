#include <iostream>
#include <vector>
using namespace std;
class perceptron
{
public:
    perceptron(vector<int> init_eval_param){
        eval_param = init_eval_param;
    }
    //users must make sure that the result is wrong before 
    //sample: the samples selected from one episode, including the count for all types
    //tag: the "correct" tag (not the predicted one)
    void train(vector<vector<int>> sample, int tag = 1){
        if(sample[0].size() != eval_param.size()){
            cout << "perceptron vector size invalid!" << endl;
            return;
        }
        for(int sample_cnt = 0; sample_cnt< sample.size(); sample_cnt++){
            for(int i = 0; i<eval_param.size(); i++){
                eval_param[i] += tag * sample[sample_cnt][i];
            }
        }
    }
public: 
    vector<int> eval_param;
};