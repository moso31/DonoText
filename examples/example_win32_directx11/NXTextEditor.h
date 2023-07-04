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
    void Render_Selection();
    void Render_TextContent();

private:
    void HandleInputs();

private:
    std::vector<std::string> m_lines;

private:
    // ��¼�к��ı��ܴﵽ�������
    float m_lineNumberWidth = 0.0f;

    // �кž����������� 4px �Ŀհ�
    float m_lineNumberPaddingX = 4.0f;

    // �ı�����ʼ����λ��
    float m_lineTextStartX;

    // �����ַ��Ĵ�С
    float m_charWidth;
    float m_charHeight;

    // ������
    float m_scrollX;
    float m_scrollY;

    std::unordered_map<Coordinate, int, CoordinateHash> m_selection;
};
