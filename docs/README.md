# DonoText

## Readme(Chinese)

一个基于 dear ImGui 的 自制代码编辑器。

主要特性：

- 代码逻辑相对简单，目前<2000行
- 基本的文本编辑
- 支持 alt 多选编辑
- HLSL语法高亮，使用线程池确保高亮逻辑快速运行。
    - 如果不喜欢也可以很方便的禁用它（目前线程池仅用于HighLightSyntax方法上）

主要缺陷：

- 不支持撤销/返回
- 没有提供优化数据结构，可能不擅长处理五万行以上级别的大文本。
- 不支持insert
- 仅支持HLSL语法高亮

## Readme(English)

A self made, dear Imgui-based TextEditor.

Features:

- Code logic relative simple,currently <2000 lines
- Basic text editing
- Support alt multi-select edit
- HLSL syntax highlight, use thread pool ensure highlight logic run fast.
    - If not prefer, it's convenient to disable(currently thread pool only used for HighLightSyntax method)
  
Major defects:

- Not support undo/redo
- Not provide optimized data structures, may not good at handling text above 50k lines.
- Not support insert
- Only support HLSL syntax highlight

![image](https://github.com/moso31/DonoText/assets/15684115/fac4eaec-524a-4936-aab8-99582d28775b)

Rainbow! 
![image](https://github.com/moso31/DonoText/assets/15684115/b03e1c3a-891f-4d04-86c9-c4ba141c64c3)
