#ifndef GOMOKU_ALGORITHMS_STATISTICAL_H_
#define GOMOKU_ALGORITHMS_STATISTICAL_H_
#include <Eigen/Dense>

namespace Gomoku::Algorithms {

struct Stats {

    template <typename Numeric>
    static Numeric Sigmoid(Numeric x) { return 1 / (1 + std::exp(-x)); };

    template <typename Numeric>
    static Numeric ReLU(Numeric x) { return std::max(x, 0); };

    template <typename Vector>
    static Vector Softmax(Eigen::Ref<Vector> logits) {
        Vector exp_logits = logits.array().exp();
        return exp_logits / exp_logits.sum();
    }
    
    // π = norm(π^(1/τ)) = softmax(log(π)/τ), 0 < τ <= 1
    static Eigen::VectorXf TempBasedProbs(Eigen::Ref<Eigen::VectorXf> logits, float temperature) {
        Eigen::VectorXd temp_logits = ((logits.array() + Epsilon).log() / temperature).cast<double>();
        return Softmax(Eigen::Ref<Eigen::VectorXd>(temp_logits)).cast<float>().unaryExpr([](float probs) { 
			return probs > Epsilon ? probs : 0; 
		});
    }

    inline static const float Epsilon = Eigen::NumTraits<float>::epsilon();

};

}

#endif // !GOMOKU_ALGORITHMS_STATISTICAL_H_
