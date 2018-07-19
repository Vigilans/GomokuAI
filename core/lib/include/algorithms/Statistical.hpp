#ifndef GOMOKU_ALGORITHMS_STATISTICAL_H_
#define GOMOKU_ALGORITHMS_STATISTICAL_H_
#include <Eigen/Dense>

namespace Gomoku::Algorithms {

struct Stats {

    template <typename Numeric>
    static Numeric Sigmoid(Numeric x) { return 1 / (1 + std::exp(-x)); };

    template <typename Numeric>
    static Numeric ReLU(Numeric x) { return std::max(x, 0); };

    static Eigen::VectorXf Softmax(Eigen::Ref<Eigen::VectorXf> logits) {
        Eigen::VectorXf exp_logits = logits.array().exp();
        return exp_logits / exp_logits.sum();
    }
    
    // ¦Ð = norm(¦Ð^(1/¦Ó)) = softmax(log(¦Ð)/¦Ó), 0 < ¦Ó <= 1
    static Eigen::VectorXf TempBasedProbs(Eigen::Ref<Eigen::VectorXf> logits, float temperature) {
        Eigen::VectorXf temp_logits = (logits.array() + Epsilon).log() / temperature;
        return Softmax(temp_logits).unaryExpr([](float probs) { return probs > Epsilon ? probs : 0; });
    }

    inline static const float Epsilon = Eigen::NumTraits<float>::epsilon();

};

}

#endif // !GOMOKU_ALGORITHMS_STATISTICAL_H_
