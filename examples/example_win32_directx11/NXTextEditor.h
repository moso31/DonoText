#pragma once
#include <vector>
#include <string>
#include <unordered_map>
#include "imgui.h"

class NXTextEditor
{
    struct Coordinate
    {
        int row;
        int col;

        bool operator==(const Coordinate& rhs) const
        {
            return row == rhs.row && col == rhs.col;
        }
    };

    // 记录单条所选文本信息，注意顺序不分前后
    // 可能是A在B前面，也可能是B在A前面。
    struct SelectionInfo
    {
        Coordinate A;
        Coordinate B;
    };

public:
    NXTextEditor();
    ~NXTextEditor() {}

    void Init();
    void Render();

    void AddSelection(int rowStart, int colStart, int rowEnd, int colEnd);
    void UpdateLastSelection(int row, int col);
    void RemoveSelection(int row, int col);
    void ClearSelection();

private:
    void Render_MainLayer();

    void Render_Selections();
    void Render_Texts();
    void Render_LineNumber();

private:
    void HandleInputs_Texts();

private:
    std::vector<std::string> m_lines;

private:
    // 记录行号文本能达到的最大宽度
    float m_lineNumberWidth = 0.0f;

    // 行号矩形两侧留出 4px 的空白
    float m_lineNumberPaddingX = 4.0f;

    float m_lineNumberWidthWithPaddingX;

    // 文本的起始像素位置
    float m_lineTextStartX;

    // 单个字符的大小
    float m_charWidth;
    float m_charHeight;

    std::vector<SelectionInfo> m_selections;
};
