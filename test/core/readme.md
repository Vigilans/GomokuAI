VS Project中包含目录添加了以下几个文件夹：

* $(SolutionDir)
* $(SolutionDir)test\core;
* $(SolutionDir)test\core\test;

这样做的效果有二：
1. 编写测试时，可以直接通过"core/.../xxx.h"来包含源代码文件。
2. 在test文件夹中，也可直接#include "pch.h"，防止预编译头在生成时出错。