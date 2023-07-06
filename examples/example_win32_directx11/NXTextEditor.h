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

    // ��¼������ѡ�ı���Ϣ��ע��˳�򲻷�ǰ��
    // ������A��Bǰ�棬Ҳ������B��Aǰ�档
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
    // ��¼�к��ı��ܴﵽ�������
    float m_lineNumberWidth = 0.0f;

    // �кž����������� 4px �Ŀհ�
    float m_lineNumberPaddingX = 4.0f;

    float m_lineNumberWidthWithPaddingX;

    // �ı�����ʼ����λ��
    float m_lineTextStartX;

    // �����ַ��Ĵ�С
    float m_charWidth;
    float m_charHeight;

    std::vector<SelectionInfo> m_selections;
};
