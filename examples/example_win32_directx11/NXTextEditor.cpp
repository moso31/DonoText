#include "NXTextEditor.h"
#include <fstream>
#include <iostream>

NXTextEditor::NXTextEditor()
{
	// 逐行读取某个文件的文本信息 
	std::ifstream file("..\\..\\imgui_demo.cpp");

	// 逐行读取文件内容到 m_lines 
	std::string line;
	while (std::getline(file, line))
	{
		m_lines.push_back(line);
	}
}

void NXTextEditor::Init()
{
    // 2023.7.4 仅支持等宽字体！其它字体感觉略有点吃性能，且没什么必要。
    // 预存储单个字符的 xy像素大小
    m_charWidth = ImGui::CalcTextSize(" ").x;
    m_charHeight = ImGui::GetTextLineHeightWithSpacing();

    // 记录行号文本能达到的最大宽度
    m_lineNumberWidth = m_charWidth * std::to_string(m_lines.size()).length();

    // 行号矩形 - 文本 之间留一个4px的空
    float paddingX = 4.0f;
    m_lineTextStartX = m_lineNumberWidth + m_lineNumberPaddingX * 2.0f + paddingX;

    AddSelection(20, 10, 12);
    AddSelection(40, 15, 14);
}

void NXTextEditor::Render()
{
	ImGui::PushID("##TextEditor");

    //ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    auto& style = ImGui::GetStyle();
	ImGui::BeginChild("TextEditor", ImVec2(0, 0), false, ImGuiWindowFlags_NoMove);

    ImGui::SetCursorPos(ImVec2(0.0f, 0.0f));
    Render_Selection();

    Render_TextContent();

	ImGui::EndChild();
	ImGui::PopID();
}

void NXTextEditor::AddSelection(int row, int col, int length)
{
    m_selection[{row, col}] = length;
}

void NXTextEditor::RemoveSelection(int row, int col)
{
    m_selection.erase({ row, col });
}

void NXTextEditor::ClearSelection()
{
    m_selection.clear();
}

void NXTextEditor::Render_Selection()
{
    // 左上角位置（绘制起始点）
    const ImVec2& windowPos = ImGui::GetWindowPos();

    ImGui::SetNextWindowPos(windowPos);
    ImGui::BeginChild("##selection");
    const auto& drawList = ImGui::GetWindowDrawList();
    for (const auto& [selectPos, selectLength] : m_selection)
    {
        // 行首坐标
        const ImVec2 linePos(windowPos.x + m_lineTextStartX, windowPos.y + m_charHeight * selectPos.row - m_scrollY);

        // 绘制所选对象的选中状态矩形
        const ImVec2 selectStartPos(linePos.x + selectPos.col * m_charWidth - m_scrollX, linePos.y);
        const ImVec2 selectEndPos(selectStartPos.x + m_charWidth * selectLength, selectStartPos.y + m_charHeight);
        drawList->AddRectFilled(selectStartPos, selectEndPos, IM_COL32(100, 100, 0, 255));
    }

    ImGui::EndChild();
}

void NXTextEditor::Render_TextContent()
{
    size_t strLineSize = std::to_string(m_lines.size()).length();

    // 左上角位置（绘制起始点）
    ImVec2 windowPos = ImGui::GetWindowPos();
    ImVec2 windowSize = ImGui::GetWindowSize();
    ImGui::SetNextWindowPos(windowPos);
    ImGui::BeginChild("##main_content", windowSize, false, ImGuiWindowFlags_HorizontalScrollbar);

    // 获取 ##main_content 的滚动条
    m_scrollX = ImGui::GetScrollX();
    m_scrollY = ImGui::GetScrollY();

    if (ImGui::IsWindowHovered())
    {
        HandleInputs();
    }

    // 逐行扫描
    for (int i = 0; i < m_lines.size(); i++)
    {
        float fLineOffsetY = i * m_charHeight; // 行偏移量

        // 获得当前行的实际像素位置（起始点）
        ImVec2 lineNumberStartPos(windowPos.x, windowPos.y + fLineOffsetY - m_scrollY);

        // 获得当前行的实际像素位置（结束点）
        // 长度 = 行号宽度 + 两侧各 4px 空白
        // 高度 = 行高（with spacing)
        ImVec2 lineNumberEndPos(lineNumberStartPos.x + m_lineNumberWidth + m_lineNumberPaddingX * 2.0f, lineNumberStartPos.y + ImGui::GetTextLineHeight());

        ImGui::BeginChild("##lineNumberBg", ImVec2(m_lineNumberWidth + m_lineNumberPaddingX * 2.0f, m_lines.size() * m_charHeight), false, ImGuiWindowFlags_NoScrollbar);
        const auto& drawList = ImGui::GetWindowDrawList();
        drawList->AddRectFilled(lineNumberStartPos, lineNumberEndPos, IM_COL32(100, 100, 0, 255));

        // 把实际的行号写上去，记得考虑 m_lineNumberPaddingX
        ImGui::SetCursorPos(ImVec2(m_lineNumberPaddingX + m_scrollX, i * m_charHeight));
        std::string strLineNumber = std::to_string(i);
        while (strLineNumber.size() < strLineSize)
            strLineNumber = " " + strLineNumber;
        ImGui::TextUnformatted(strLineNumber.c_str());
        ImGui::EndChild();

        // 然后在行号右侧写实际的文本
        // 使用 m_lineTextStartX，这样可以在 行号矩形 - 文本 之间留一个4px的空
        ImGui::SetCursorPos(ImVec2(m_lineTextStartX, i * m_charHeight));
        ImGui::TextUnformatted(m_lines[i].c_str());
    }

    ImGui::EndChild();
}

void NXTextEditor::HandleInputs()
{
    ImGuiIO& io = ImGui::GetIO();
    {
        bool bClicked = ImGui::IsMouseClicked(ImGuiMouseButton_Left);
        bool bDoubleClicked = ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left);

        if (bClicked)
        {
            const auto& mousePos = ImGui::GetMousePos();
            const auto& windowPos = ImGui::GetWindowPos();
            const ImVec2 relativeWindowPos(mousePos.x - windowPos.x - m_lineTextStartX, mousePos.y - windowPos.y);

            // 获取在整个文本显示区中的相对位置
            const ImVec2 relativePos(m_scrollX + relativeWindowPos.x, m_scrollY + relativeWindowPos.y);

            // 计算出行列号
            int row = relativePos.y / m_charHeight;
            int col = relativePos.x / m_charWidth;

            // 验证对应字符
            if (col < m_lines[row].size())
                std::cout << m_lines[row][col];

            ClearSelection();
        }

        if (bDoubleClicked)
        {
            const auto& mousePos = ImGui::GetMousePos();
            const auto& windowPos = ImGui::GetWindowPos();
            const ImVec2 relativeWindowPos(mousePos.x - windowPos.x - m_lineTextStartX, mousePos.y - windowPos.y);

            // 获取在整个文本显示区中的相对位置
            const ImVec2 relativePos(m_scrollX + relativeWindowPos.x, m_scrollY + relativeWindowPos.y);

            // 计算出行列号
            int row = relativePos.y / m_charHeight;
            int col = relativePos.x / m_charWidth;

            // 抓取对应字符
            if (col < m_lines[row].size())
            {
                // 从当前位置开始向左右扫描，直到遇到空格或者换行符
                int left = col;
                int right = col;
                while (left > 0 && m_lines[row][left] != ' ' && m_lines[row][left] != '\n')
                    left--;
                while (right < m_lines[row].size() && m_lines[row][right] != ' ' && m_lines[row][right] != '\n')
                    right++;
                // 计算出选中的长度
                int length = right - left - 1;
                // 添加选中
                AddSelection(row, left + 1, length);
            }
        }
    }
}
