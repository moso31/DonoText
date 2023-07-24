# DonoText

## Readme_Chinese

一个基于 dear ImGui 的 自制代码编辑器。

主要特性：

- 轻量级，少于 3000 行且仅两个文件，可以快速集成到各种项目
- 支持多选项卡编辑
- 允许 alt 多选的文本编辑
- 支持较简单的 HLSL 语法高亮，使用线程池确保高亮逻辑快速运行。

主要缺陷：

- 仍处于实验阶段，出现bug甚至崩溃的概率较高。
- 不支持撤销/返回、insert。
- 不支持双字节，输入的所有双字节字符都会被替换成'?'
- 没有提供优化数据结构，处理万行以上级别的大文本可能响应缓慢。

## Readme_English (translated by ChatGPT)

An self-made code editor based on dear ImGui.

Features:

- Lightweight, less than 3000 lines and only two files, can be integrated into projects easily
- Allow multiple tabs editing
- Support alt multi-select text editing
- Support basic HLSL syntax highlighting, using a thread pool to ensure highlighting logic runs fast.

Major defects:

- Still at the experimental stage, the probability of bugs and even crashes is relatively high.
- Do not support undo/redo, insert.
- Do not support double byte, all double byte characters entered will be replaced with '?'
- Does not provide optimized data structures, handling text above 10k lines may be slow.

## Preview

![image](https://github.com/moso31/DonoText/assets/15684115/9c65ccf1-1045-4f5b-a3c2-19f8ab24ce44)

![image](https://github.com/moso31/DonoText/assets/15684115/6ae49ef4-80b8-4a05-9385-742c26270c1b)

