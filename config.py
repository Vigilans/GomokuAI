MCTS_CONFIG = {
    "c_puct": 5.0,
    "c_iterations": 400,
}

DATA_CONFIG = {
    # one of "random", "botzone", "mcts", "alpha_zero"
    "schedule": {
        "supervisor": ("traditional_mcts", { 
            "c_puct": MCTS_CONFIG["c_puct"], 
            "c_iterations": 20000
        }),
        "candidates": [
            ("random_mcts", MCTS_CONFIG),
            ("rave_mcts", MCTS_CONFIG),
            ("botzone", {"program": "deeeeep"}),
            ("botzone", {"program": "genm"}),
            None
        ]
    },
    "process_num": 2,
    "buffer_size": 10000,
    "data_path": "./data/training_data",
    "data_files": [
        # after read out,
        # file name will be marked ".consumed"
    ]
}

TRAINING_CONFIG = {
    "num_epoches": 5,
    "batch_size": 512,
    "learning_rate": 2e-3,
    "momentum": 0.9,
    "kl_target": 0.02,
    "eval_period": 100,
    "eval_rounds": 11,
    "model_file": "latest",
    "model_path": "./data/trained_models"
}
