VS Project中包含目录添加了以下两个文件夹：

* $(SolutionDir)core\;
* $(ProjectDir)

这样做的效果有二：
1. 编写测试时，可以直接通过"lib/include/xxx.h"来包含源代码文件。
2. 在test文件夹中，也可直接#include "pch.h"，防止预编译头在生成时出错。