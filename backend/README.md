# 编译器后端

## 组织结构

`Backend` 类负责后端的整体过程。

后端代码的运行方式：

```
前端产生的 mir::MirPackage
    --> Backend::new(mir::MirPackage)
        --> Backend::do_mir_optimization()
            --> 按注册顺序执行所有 MirOptimizePass::optimize_mir
        --> Backend::do_mir_to_arm_transform()
            --> Codegen::generate_code()
                --> 执行 Mir 到 ARM 汇编的直接翻译（无优化）
            <-- arm::ArmCode arm_code
        --> Backend::do_arm_optimization()
            --> 按注册顺序调用所有 ArmOptimizePass::optimize_arm
    <-- arm::ArmCode return_value
优化后的汇编代码
--> 汇编代码转换成字符串
--> 输出文件
```

## 编写优化 pass 的注意事项

用于优化 Mir 的 Pass 应继承 `MirOptimizePass`；用于优化 ARM 代码的 Pass 应继承 `ArmOptimizePass`。两种 Pass 均应使用 `Backend::add_pass()` 函数注册到后端中。

优化和分析 pass 中产生的数据应当存放在 `optimize_mir` 或 `optimize_arm` 函数传入的 `extra_data_repo` 对象中。当然，数据的键不能重名。

