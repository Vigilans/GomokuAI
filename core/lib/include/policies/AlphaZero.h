#ifndef GOMOKU_POLICY_ALPHAZERO_H_
#define GOMOKU_POLICY_ALPHAZERO_H_
#include "../MCTS.h"
#include "../algorithms/Default.h"
#include <tensorflow/core/public/session.h>
#include <tensorflow/core/platform/env.h>
#include <tensorflow/core/framework/graph_def_util.h>

namespace Gomoku::Policies {

namespace tf = tensorflow;

class AlphaZeroPolicy : public Policy {
public:
    using Default = Algorithms::Default; // 引入默认算法

    AlphaZeroPolicy(std::string model_path, double puct = C_PUCT) :
        Policy(nullptr, nullptr, [this](auto& board) { return networkEvaluate(board); }, nullptr, puct), 
        m_modelPath(std::move(model_path)) {

    }

    EvalResult networkEvaluate(Board& board) {
        auto init_player = board.m_curPlayer;
        std::vector<tf::Tensor> outputs;

        m_session->Run({
            {"", tf::Tensor()}
            }, { "" }, {}, outputs);
    }

    virtual void prepare(Board& board) override {
        Policy::prepare(board);
    }

    virtual void reset() override {
        auto status_load = tf::ReadBinaryProto(tf::Env::Default(), m_modelPath, &m_graphDef);
        if (!status_load.ok()) {
            throw std::runtime_error(status_load.ToString());
        }
        auto status_create = m_session->Create(m_graphDef);
        if (!status_create.ok()) {
            throw std::runtime_error(status_create.ToString());
        }
    }

public:
    std::string m_modelPath;
    std::unique_ptr<tf::Session> m_session;
    tf::GraphDef m_graphDef;
};

}

#endif // !GOMOKU_POLICY_ALPHAZERO_H_
