# DonoText

## Readme_Chinese

一个基于 dear ImGui 的 自制代码编辑器。

主要特性：

- 轻量级，少于3000行，仅两个文件，可以快速集成到各种项目
- 支持 alt 多选的文本编辑
- HLSL语法高亮，使用线程池确保高亮逻辑快速运行。
    - 你可以很简单的将其改回单线程（目前线程池仅用于HighLightSyntax方法）

主要缺陷：

- 不支持撤销/返回、insert。
- 不支持双字节
- 没有提供优化数据结构，处理万行以上级别的大文本可能响应缓慢。
- 语法高亮的支持相当简陋

## Readme_English (translated by ChatGPT)

An self-made code editor based on dear ImGui.

Features:

- Lightweight, less than 3000 lines, only two c++ files, so can integrate into your projects easily
- Support alt multi-select text editing
- HLSL syntax highlight,use thread pool ensure highlight logic run fast.
- If not prefer my thread pool, you can disable it easily(currently thread pool only used for HighLightSyntax method)

Defects:

- Not support undo/redo, insert.
- Not support double byte
- Not provide optimized data structures,handling text above 10k lines may be slow.
- Very basic syntax highlight support.

![image](https://github.com/moso31/DonoText/assets/15684115/7327b7dd-810d-4732-88cc-c6f44bda0892)

![image](https://github.com/moso31/DonoText/assets/15684115/6ae49ef4-80b8-4a05-9385-742c26270c1b)

