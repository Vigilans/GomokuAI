#ifndef GOMOKU_ALGORITHMS_STATISTICAL_H_
#define GOMOKU_ALGORITHMS_STATISTICAL_H_
#include "MonteCarlo.hpp"

namespace Gomoku::Algorithms {

struct Statistical {

    static Eigen::VectorXf Softmax(Eigen::Ref<Eigen::VectorXf> logits) {
        Eigen::VectorXf exp_logits = logits.array().exp();
        return exp_logits / exp_logits.sum();
    }

    static Eigen::VectorXf TempBasedProbs(Eigen::Ref<Eigen::VectorXf> logits, float temperature) {
        Eigen::VectorXf temp_logits = (logits.array() + 1e-10).log() / temperature;
        return Softmax(temp_logits);
    }
};

}

#endif // !GOMOKU_ALGORITHMS_STATISTICAL_H_
