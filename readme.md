# Gomoku AI Framework

## Usage

### Agents

Run any file(except `utils.py`) under agents module to run as [botzone](https://botzone.org.cn/)-adapted program.

```
python -m agents.human
```

### Network

Run train.py for training network in a pipeline.
```
python -m network.train
```

Run data_helper.py for simply generating AlphaZero self-play data.
```
python -m network.data_helper
```

### Core

Switch to `Visual Studio`/`CLion`/`CMake for Visual Studio` for a better developing experience.

* Use `CoreInterface` project for generating botzone-adapted executable.
* Use `CorePyExt` project for generating Python Extension module library.

## Prerequisities

### Toolchain

* C++ >=17
* Python >=3.6
* CMake >=3.8
* Visual Studio >=15.7.1 or GCC >=7.3
* [Vcpkg](https://github.com/Microsoft/vcpkg)

### Libraries

#### C++

[Vcpkg](https://github.com/Microsoft/vcpkg) are strongly recommended for managing library dependencies.

* Eigen >=3
* nlohmann-json
* pybind11 >= 2.11
* gtest
* [tensorflow_cc](https://github.com/FloopCZ/tensorflow_cc)

#### Python

* numpy
* h5py
* tensorflow >= 1.7
* keras

## To-dos

Any kind of contributions are welcomed!

### Agents
- [ ] Implement BotzoneAgent's keep_alive version
- [ ] [Network interface](https://wiki.botzone.org/index.php?title=%E6%9C%AC%E5%9C%B0AI) for botzone
- [ ] Directly run botzone's WebAssembly program as an Agent
- [ ] PygameAgent for human-friendly interaction with bots

### Network
- [ ] Implement Kera's version of PolicyValueNetwork correctly

### Core
- [ ] Try to make RAVE policy behave well
- [ ] Implement PoolRAVE policy
- [ ] **Implement AlphaBeta Search and integrate with existing structure**
- [ ] Make Policy class more scalable
- [ ] Adaption for more varieties of board games

### Others
- [ ] More test code
