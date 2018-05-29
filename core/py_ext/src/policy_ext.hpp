#include "pch.h"
#include "lib/include/policies/Random.h"
#include "lib/include/policies/PoolRAVE.h"
#include "lib/include/policies/Traditional.h"

inline void Policy_Ext(py::module& mod) {
    using namespace Gomoku;
    using namespace Gomoku::Policies;
    using namespace std;

    py::class_<RandomPolicy, Policy, std::shared_ptr<RandomPolicy>>
        (mod, "RandomPolicy", "Random policy with averaged mutliple rollouts")
        .def(py::init<double, size_t>(),
            py::arg("c_puct") = C_PUCT,
            py::arg("c_rollouts") = 5
        )
        .def("__repr__", [](const RandomPolicy& p) { 
            return py::str(
                "RandomPolicy(c_puct: {}, c_rollouts: {}, init_acts: {})"
            ).format(p.c_puct, p.c_rollouts, p.m_initActs); 
        });


    py::class_<PoolRAVEPolicy, Policy, std::shared_ptr<PoolRAVEPolicy>>
        (mod, "PoolRAVEPolicy", "PoolRAVE policy with MC-RAVE algorithm")
        .def(py::init<double, double>(),
            py::arg("c_puct") = 2,
            py::arg("c_bias") = 0
        )
        .def("__repr__", [](const PoolRAVEPolicy& p) { 
            return py::str(
                "PoolRAVEPolicy(c_puct: {}, c_bias: {}, init_acts: {})"
            ).format(p.c_puct, p.c_bias, p.m_initActs); 
        });


    py::class_<TraditionalPolicy, Policy, std::shared_ptr<TraditionalPolicy>>
        (mod, "TraditionalPolicy", "Traditional policy with MC + Pattern Matching algorithm")
        .def(py::init<double, double, bool>(),
            py::arg("c_puct") = C_PUCT,
            py::arg("c_bias") = 0,
            py::arg("use_rave") = false
        )
        .def("__repr__", [](const TraditionalPolicy& p) { 
            if (p.c_useRave) {
                return py::str(
                    "TraditionalPolicy(c_puct: {}, c_bias: {}, init_acts: {}, cached_acts: {})"
                ).format(p.c_puct, p.c_bias, p.m_initActs, p.m_cachedActs);
            } else {
                return py::str(
                    "TraditionalPolicy(c_puct: {}, init_acts: {}, cached_acts: {})"
                ).format(p.c_puct, p.m_initActs, p.m_cachedActs);
            }
        });
}