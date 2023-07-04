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

    struct CoordinateHash
    {
        std::size_t operator()(const Coordinate& key) const
        {
            return static_cast<uint64_t>(key.row) << 32 | key.col;
        }
    };

public:
    NXTextEditor();
    ~NXTextEditor() {}

    void Init();
    void Render();

    void AddSelection(int row, int col, int length);
    void RemoveSelection(int row, int col);
    void ClearSelection();

private:
    void Render_MainText(const std::string& strLine);
    void Render_Selection();
    void Render_LineNumber();

private:
    void HandleInputs();

private:
    std::vector<std::string> m_lines;

private:
    // 记录行号文本能达到的最大宽度
    float m_lineNumberWidth = 0.0f;

    // 行号矩形两侧留出 4px 的空白
    float m_lineNumberPaddingX = 4.0f;

    // 文本的起始像素位置
    float m_lineTextStartX;

    float m_charWidth;
    float m_charHeight;

    std::unordered_map<Coordinate, int, CoordinateHash> m_selection;
};
