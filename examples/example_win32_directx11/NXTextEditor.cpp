#include "NXTextEditor.h"
#include <fstream>
#include <iostream>

NXTextEditor::NXTextEditor()
{
	// 逐行读取某个文件的文本信息 
	std::ifstream file("..\\..\\imgui_demo.cpp");
	//std::ifstream file("..\\..\\license.txt");
    //std::ifstream file("D:\\Users\\Administrator\\Source\\Repos\\DonoText\\imgui_demo.cpp");

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

    m_lineNumberWidthWithPaddingX = m_lineNumberWidth + m_lineNumberPaddingX * 2.0f;

    // 行号矩形 - 文本 之间留一个4px的空
    float paddingX = 4.0f;
    m_lineTextStartX = m_lineNumberWidthWithPaddingX + paddingX;

    AddSelection(10, 10, 10, 25);
    AddSelection(40, 15, 14, 10);
}

void NXTextEditor::Render()
{
    //ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::Begin("TextEditor", (bool*)true, ImGuiWindowFlags_NoMove);

    ImGui::BeginChild("##main_layer");
    Render_MainLayer();
    ImGui::EndChild();

    ImGui::End();
    //ImGui::PopStyleVar();
}

void NXTextEditor::AddSelection(int rowStart, int colStart, int rowEnd, int colEnd)
{
    m_selections.push_back({ {rowStart, colStart}, {rowEnd, colEnd} });
}

void NXTextEditor::UpdateLastSelection(int row, int col)
{
    if (m_selections.empty())
        return;

    auto& selection = m_selections.back();
    selection.B = { row, col };
}

void NXTextEditor::RemoveSelection(int row, int col)
{
}

void NXTextEditor::ClearSelection()
{
    m_selections.clear();
}

void NXTextEditor::Render_MainLayer()
{
    const ImVec2& windowPos = ImGui::GetWindowPos();
    const ImVec2& windowSize = ImGui::GetWindowSize();

    ImGui::SetCursorPos(ImVec2(m_lineTextStartX, 0.0f));
    ImGui::BeginChild("##text_content", ImVec2(windowSize.x - m_lineTextStartX, windowSize.y), false, ImGuiWindowFlags_HorizontalScrollbar);
    HandleMouseInputs_Texts();
    Render_Selections();
    Render_Texts();
    float scrollY_textContent = ImGui::GetScrollY();
    float scrollBarHeight = ImGui::GetScrollMaxY() > 0.0f ? ImGui::GetStyle().ScrollbarSize : 0.0f;
    ImGui::EndChild();

    ImGui::SetCursorPos(ImVec2(0.0f, 0.0f));
    ImGui::BeginChild("##line_number", ImVec2(m_lineNumberWidthWithPaddingX, windowSize.y - scrollBarHeight), false, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    ImGui::SetScrollY(scrollY_textContent);
    Render_LineNumber();
    ImGui::EndChild();
}

void NXTextEditor::Render_Selections()
{
    const ImVec2& windowPos = ImGui::GetWindowPos();
    const auto& drawList = ImGui::GetWindowDrawList();

    float scrollX = ImGui::GetScrollX();
    float scrollY = ImGui::GetScrollY();

    for (const auto& selection : m_selections)
    {
        // 判断 A B 的前后顺序
        const Coordinate& A = selection.A;
        const Coordinate& B = selection.B;
        const Coordinate& fromPos = A.row < B.row ? A : A.row > B.row ? B : A.col <= B.col ? A : B;
        const Coordinate& toPos = (A.row == fromPos.row && A.col == fromPos.col) ? B : A;

        // 判断 A B 是否在同一行
        const bool bSameLine = fromPos.row == toPos.row;
        if (bSameLine)
        {
            // 行首坐标
            ImVec2 linePos(windowPos.x, windowPos.y + m_charHeight * fromPos.row - scrollY);

            // 绘制所选对象的选中状态矩形
            ImVec2 selectStartPos(linePos.x + fromPos.col * m_charWidth - scrollX, linePos.y);
            ImVec2 selectEndPos(linePos.x + toPos.col * m_charWidth - scrollX, linePos.y + m_charHeight);
            drawList->AddRectFilled(selectStartPos, selectEndPos, IM_COL32(100, 100, 0, 255));

            // 在末尾的位置，绘制闪烁条 每秒钟闪烁一次
            if (fmod(ImGui::GetTime(), 1.0f) > 0.5f)
                drawList->AddLine(ImVec2(selectEndPos.x, selectStartPos.y), selectEndPos, IM_COL32(255, 255, 0, 255), 1.0f);
        }
        else
        {
            // 绘制首行，先确定首行字符长度
            const int firstLineLength = m_lines[fromPos.row].length() - fromPos.col;

            // 行首坐标
            ImVec2 linePos(windowPos.x, windowPos.y + m_charHeight * fromPos.row - scrollY);
            // 绘制所选对象的选中状态矩形
            ImVec2 selectStartPos(linePos.x + fromPos.col * m_charWidth - scrollX, linePos.y);
            ImVec2 selectEndPos(linePos.x + firstLineLength * m_charWidth - scrollX, linePos.y + m_charHeight);
            drawList->AddRectFilled(selectStartPos, selectEndPos, IM_COL32(100, 100, 0, 255));

            // 绘制中间行
            for (int i = fromPos.row + 1; i < toPos.row; ++i)
            {
                // 行首坐标
                linePos = ImVec2(windowPos.x, windowPos.y + m_charHeight * i - scrollY);
                // 绘制所选对象的选中状态矩形
                selectStartPos = ImVec2(linePos.x - scrollX, linePos.y);
                selectEndPos = ImVec2(linePos.x + m_lines[i].length() * m_charWidth - scrollX, linePos.y + m_charHeight);
                drawList->AddRectFilled(selectStartPos, selectEndPos, IM_COL32(100, 100, 0, 255));
            }

            // 绘制尾行，先确定尾行字符长度
            int lastLineLength = toPos.col;
            // 行首坐标
            linePos = ImVec2(windowPos.x, windowPos.y + m_charHeight * toPos.row - scrollY);
            // 绘制所选对象的选中状态矩形
            selectStartPos = ImVec2(linePos.x - scrollX, linePos.y);
            selectEndPos = ImVec2(linePos.x + lastLineLength * m_charWidth - scrollX, linePos.y + m_charHeight);
            drawList->AddRectFilled(selectStartPos, selectEndPos, IM_COL32(100, 100, 0, 255));

            // 在末尾的位置，绘制闪烁条 每秒钟闪烁一次
            if (fmod(ImGui::GetTime(), 1.0f) > 0.5f)
                drawList->AddLine(ImVec2(selectEndPos.x, selectStartPos.y), selectEndPos, IM_COL32(255, 255, 0, 255), 1.0f);
        }
    }
}

void NXTextEditor::Render_Texts()
{
    for (int i = 0; i < m_lines.size(); i++)
    {
        const auto& strLine = m_lines[i];
        ImGui::TextUnformatted(strLine.c_str());
    }
}

void NXTextEditor::Render_LineNumber()
{
    const ImVec2& windowPos = ImGui::GetWindowPos();
    const ImVec2& windowSize = ImGui::GetWindowSize();
    const auto& drawList = ImGui::GetWindowDrawList();

    drawList->AddRectFilled(windowPos, ImVec2(windowPos.x + windowSize.x, windowPos.y + windowSize.y), IM_COL32(100, 100, 0, 255));

    size_t strLineSize = std::to_string(m_lines.size()).length();
    for (int i = 0; i < m_lines.size(); i++)
    {
        std::string strLineNumber = std::to_string(i);
        while (strLineNumber.size() < strLineSize)
            strLineNumber = " " + strLineNumber;

        ImGui::SetCursorPosX(m_lineNumberPaddingX);
        ImGui::TextUnformatted(strLineNumber.c_str());
    }
}

void NXTextEditor::HandleMouseInputs_Texts()
{
    if (ImGui::IsMouseReleased(ImGuiMouseButton_Left))
    {
        m_bIsSelecting = false;
        return;
    }

    float scrollX = ImGui::GetScrollX();
    float scrollY = ImGui::GetScrollY();

    const auto& mousePos = ImGui::GetMousePos();

    // 获取鼠标在文本显示区中的相对位置
    const auto& windowPos = ImGui::GetWindowPos();
    const ImVec2 relativeWindowPos(mousePos.x - windowPos.x, mousePos.y - windowPos.y);
    const ImVec2 relativePos(scrollX + relativeWindowPos.x, scrollY + relativeWindowPos.y);

    // 获取文本内容区域的矩形
    const ImVec2& contentAreaMin(windowPos);
    const ImVec2& textContentArea(ImGui::GetContentRegionAvail());
    const ImVec2 contentAreaMax(contentAreaMin.x + textContentArea.x, contentAreaMin.y + textContentArea.y);

    // 检查鼠标点击是否在文本内容区域内
    bool isMouseInContentArea = mousePos.x >= contentAreaMin.x && mousePos.x <= contentAreaMax.x &&
        mousePos.y >= contentAreaMin.y && mousePos.y <= contentAreaMax.y;

    if (!isMouseInContentArea)
        return;

    if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
    {
        // 计算出行列号
        int row = relativePos.y / m_charHeight;
        float fCol = relativePos.x / m_charWidth;

        // 手感优化：实际点击位置的 列坐标 超过当前字符的50% 时，认为是下一个字符
        int col = fCol + 0.5f;

        // 约束行列号范围
        row = std::max(0, std::min(row, (int)m_lines.size() - 1));
        col = std::max(0, std::min(col, (int)m_lines[row].size()));

        // 抓取对应字符
        if (col < m_lines[row].size())
        {
            // 从当前位置开始向左右扫描，直到遇到空格或者换行符
            int left = col;
            int right = col;
            while (left > 0 && m_lines[row][left] != ' ' && m_lines[row][left] != '\n') left--;
            while (right < m_lines[row].size() && m_lines[row][right] != ' ' && m_lines[row][right] != '\n') right++;

            // 计算出选中的长度
            int length = right - left - 1;

            // 添加选中
            AddSelection(row, left + 1, row, right);
        }
    }
    else if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
    {
        // 计算出行列号
        int row = relativePos.y / m_charHeight;
        float fCol = relativePos.x / m_charWidth;

        // 手感优化：实际点击位置的 列坐标 超过当前字符的50% 时，认为是下一个字符
        int col = fCol + 0.5f;

        // 约束行列号范围
        row = std::max(0, std::min(row, (int)m_lines.size() - 1));
        col = std::max(0, std::min(col, (int)m_lines[row].size()));

        // test: 打印对应字符
        //if (col < m_lines[row].size()) std::cout << m_lines[row][col];

        ClearSelection();
        AddSelection(row, col, row, col);

        m_bIsSelecting = true;
    }
    else if (m_bIsSelecting && ImGui::IsMouseDragging(ImGuiMouseButton_Left))
    {
        // 处理鼠标拖拽
        // 计算出行列号
        int row = relativePos.y / m_charHeight;
        float fCol = relativePos.x / m_charWidth;

        // 手感优化：实际点击位置的 列坐标 超过当前字符的50% 时，认为是下一个字符
        int col = fCol + 0.5f;

        // 约束行列号范围
        row = std::max(0, std::min(row, (int)m_lines.size() - 1));
        col = std::max(0, std::min(col, (int)m_lines[row].size()));

        // 添加选中
        UpdateLastSelection(row, col);
    }
}

void NXTextEditor::HandleKeyInputs_Texts()
{
}
