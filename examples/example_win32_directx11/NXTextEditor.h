﻿#pragma once
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

        // 自动对 A B 排序
        SelectionInfo(const Coordinate& A, const Coordinate& B) : L(A < B ? A : B), R(A < B ? B : A), flickerAtFront(A > B) {}

        bool operator==(const SelectionInfo& rhs) const
        {
            return L == rhs.L && R == rhs.R;
        }

        bool operator==(const Coordinate& rhs) const
        {
            return L == rhs && R == rhs;
        }

        // 检测另一个 SelectionInfo 是否是当前 SelectionInfo 的子集
        bool Include(const SelectionInfo& selection) const
        {
            return L <= selection.L && R >= selection.R;
        }

        // 检测 Coordinate 是否在当前 SelectionInfo 内
        bool Include(const Coordinate& X) const
        {
            return L <= X && R >= X;
        }

        Coordinate L;
        Coordinate R;
        bool flickerAtFront = false;
    };

public:
    NXTextEditor();
    ~NXTextEditor() {}

    void Init();
    void Render();

    void AddSelection(const Coordinate& A, const Coordinate& B);
    void RemoveSelection(const SelectionInfo& removeSelection);
    void ClearSelection();

public:
    void Enter(ImWchar c);
    void Backspace();

private:
    void Render_MainLayer();
    void Render_DebugLayer();

    void RenderSelections();
    void RenderTexts();
    void RenderLineNumber();

    void RenderSelection(const SelectionInfo& selection);
    void SelectionsOverlayCheck(bool bIsDoubleClick);

private:
    void Render_OnMouseInputs();

    void RenderTexts_OnMouseInputs();
    void RenderTexts_OnKeyInputs();

    void MoveUp(bool bShift);
    void MoveDown(bool bShift);
    void MoveLeft(bool bShift);
    void MoveRight(bool bShift);

private:
    int min(int x, int y) { return x < y ? x : y; }
    int max(int x, int y) { return x < y ? x : y; }

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

    // 闪烁计时
    double m_flickerDt = 0.0f;
    bool m_bResetFlickerDt = false;

    std::vector<SelectionInfo> m_selections;

    bool m_bIsSelecting = false;

    Coordinate m_activeSelectionDown;
    Coordinate m_activeSelectionMove;

    bool m_bNeedFocusOnText = true;
};
