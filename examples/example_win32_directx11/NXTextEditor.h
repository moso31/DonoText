#pragma once
#include <vector>
#include <string>
#include <unordered_map>
#include "imgui.h"

class NXTextEditor
{
    struct Coordinate
    {
        Coordinate() : row(0), col(0) {}
        Coordinate(int r, int c) : row(r), col(c) {}

        bool operator==(const Coordinate& rhs) const
        {
            return row == rhs.row && col == rhs.col;
        }

        bool operator!=(const Coordinate& rhs) const
        {
            return !(*this == rhs);
        }

        bool operator<(const Coordinate& rhs) const
        {
            return row < rhs.row || (row == rhs.row && col < rhs.col);
        }

        bool operator>(const Coordinate& rhs) const
        {
            return row > rhs.row || (row == rhs.row && col > rhs.col);
        }

        bool operator<=(const Coordinate& rhs) const
        {
            return *this < rhs || *this == rhs;
        }

        bool operator>=(const Coordinate& rhs) const
        {
            return *this > rhs || *this == rhs;
        }

        int row;
        int col;
    };

    // 记录单条所选文本信息 from L to R
    struct SelectionInfo
    {
        SelectionInfo() {}
        SelectionInfo(const Coordinate& left, const Coordinate& right) : L(left), R(right) {}
        SelectionInfo(const Coordinate& left, const Coordinate& right, bool flickerAtFront) : L(left), R(right), flickerAtFront(flickerAtFront) {}

        // 检测另一个 SelectionInfo 是否是当前 SelectionInfo 的子集
        bool Include(const SelectionInfo& selection) const
        {
            return L <= selection.L && R >= selection.R;
        }

        Coordinate L;
        Coordinate R;
        bool flickerAtFront = false;
        bool isDraging = false;
    };

public:
    NXTextEditor();
    ~NXTextEditor() {}

    void Init();
    void Render();

    void AddSelection(const Coordinate& A, const Coordinate& B);
    void DragSelection(SelectionInfo& selection, const Coordinate& newPos);
    void RemoveSelection(int row, int col);
    void ClearSelection();

public:
    void Enter(ImWchar c);
    void Backspace();

private:
    void Render_MainLayer();

    void RenderSelections();
    void RenderTexts();
    void RenderLineNumber();

    void SelectionsOverlayCheck(const SelectionInfo& selection);

private:
    void Render_OnMouseInputs();
    void Render_OnMouseLeftRelease();

    void RenderTexts_OnMouseInputs();
    void RenderTexts_OnKeyInputs();

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

    bool m_bIsSelecting = false;

    Coordinate m_activeSelectionDown;
    Coordinate m_activeSelectionMove;
};
