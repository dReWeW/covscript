# Covariant Script Vulnerabilities & Exposures #
## CSVE-2018-01-01 ##
+ 触发方法：在三目运算符中进行递归
+ 漏洞原理：CovScript在计算三目运算符时会将两个目标值全部计算从而可能造成无限递归从而触发栈溢出
+ 漏洞状态：已修复
+ 影响范围：CovScript 1.1.0-1.2.1(Unstable)
## CSVE-2018-01-02 ##
+ 触发方法：在函数调用后直接使用点运算符
+ 漏洞原理：CovScript编译器对运算符的推断出现了误判导致AST构建错误，CovScript解释器未检查AST格式的情况下直接解释导致段错误
+ 漏洞状态：已修复
+ 影响范围：CovScript 1.2.0-1.2.1(Unstable)
## CSVE-2018-01-03 ##
+ 触发方法：在三目运算符中使用两个常量
+ 漏洞原理：CovScript优化器在对AST进行剪枝时会误剪掉符合条件的三目运算符的两个分支，导致AST格式丢失报错
+ 漏洞状态：已修复
+ 影响范围：CovScript 1.2.1(Unstable)
## CSVE-2018-01-04 ##
+ 触发方式：嵌套Import并调用其中的函数
+ 漏洞原理：CovScript在去年八月的大重构中忽略了更换嵌套的Import实例的宿主环境(注：宿主环境在解释完需要引入的包之后会自动销毁)，导致资源不可用
+ 漏洞状态：已修复
+ 影响范围：CovScript 1.1.1-1.2.1(Unstable)