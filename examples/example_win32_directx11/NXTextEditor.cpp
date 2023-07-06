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

    //AddSelection(20, 10, 22, 10);
    AddSelection(40, 15, 14, 10);
}

void NXTextEditor::Render()
{
    //ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::Begin("TextEditor", (bool*)true);

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

void NXTextEditor::UpdateSelectionEnd(int rowEnd, int colEnd)
{
}

void NXTextEditor::RemoveSelection(int row, int col)
{
}

void NXTextEditor::ClearSelection()
{
    m_selections.clear();
}

void NXTextEditor::Render_Selection()
{
    // 左上角位置（绘制起始点）
    const ImVec2& windowPos = ImGui::GetWindowPos();

    ImGui::SetNextWindowPos(windowPos);
    ImGui::BeginChild("##selection");
    const auto& drawList = ImGui::GetWindowDrawList();
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
            ImVec2 linePos(windowPos.x + m_lineTextStartX, windowPos.y + m_charHeight * fromPos.row - m_scrollY);

            // 绘制所选对象的选中状态矩形
            ImVec2 selectStartPos(linePos.x + fromPos.col * m_charWidth - m_scrollX, linePos.y);
            ImVec2 selectEndPos(linePos.x + toPos.col * m_charWidth - m_scrollX, linePos.y + m_charHeight);
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
            ImVec2 linePos(windowPos.x + m_lineTextStartX, windowPos.y + m_charHeight * fromPos.row - m_scrollY);
            // 绘制所选对象的选中状态矩形
            ImVec2 selectStartPos(linePos.x + fromPos.col * m_charWidth - m_scrollX, linePos.y);
            ImVec2 selectEndPos(linePos.x + firstLineLength * m_charWidth - m_scrollX, linePos.y + m_charHeight);
            drawList->AddRectFilled(selectStartPos, selectEndPos, IM_COL32(100, 100, 0, 255));

            // 绘制中间行
            for (int i = fromPos.row + 1; i < toPos.row; ++i)
            {
                // 行首坐标
                linePos = ImVec2(windowPos.x + m_lineTextStartX, windowPos.y + m_charHeight * i - m_scrollY);
                // 绘制所选对象的选中状态矩形
                selectStartPos = ImVec2(linePos.x - m_scrollX, linePos.y);
                selectEndPos = ImVec2(linePos.x + m_lines[i].length() * m_charWidth - m_scrollX, linePos.y + m_charHeight);
                drawList->AddRectFilled(selectStartPos, selectEndPos, IM_COL32(100, 100, 0, 255));
            }

            // 绘制尾行，先确定尾行字符长度
            int lastLineLength = toPos.col;
            // 行首坐标
            linePos = ImVec2(windowPos.x + m_lineTextStartX, windowPos.y + m_charHeight * toPos.row - m_scrollY);
            // 绘制所选对象的选中状态矩形
            selectStartPos = ImVec2(linePos.x - m_scrollX, linePos.y);
            selectEndPos = ImVec2(linePos.x + lastLineLength * m_charWidth - m_scrollX, linePos.y + m_charHeight);
            drawList->AddRectFilled(selectStartPos, selectEndPos, IM_COL32(100, 100, 0, 255));

            // 在末尾的位置，绘制闪烁条 每秒钟闪烁一次
            if (fmod(ImGui::GetTime(), 1.0f) > 0.5f)
                drawList->AddLine(ImVec2(selectEndPos.x, selectStartPos.y), selectEndPos, IM_COL32(255, 255, 0, 255), 1.0f);
        }
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

    ImGui::BeginChild("##lineNumberBg", ImVec2(m_lineNumberWidthWithPaddingX, m_lines.size() * m_charHeight), false, ImGuiWindowFlags_NoScrollbar);
    const auto& drawList = ImGui::GetWindowDrawList();
    drawList->AddRectFilled(windowPos, ImVec2(windowPos.x + ImGui::GetWindowSize().x, windowPos.y + ImGui::GetWindowSize().y), IM_COL32(100, 100, 0, 255));
    ImGui::EndChild();

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
        ImVec2 lineNumberEndPos(lineNumberStartPos.x + m_lineNumberWidthWithPaddingX, lineNumberStartPos.y + ImGui::GetTextLineHeight());

        ImGui::BeginChild("##lineNumber", ImVec2(m_lineNumberWidthWithPaddingX, m_lines.size() * m_charHeight), false, ImGuiWindowFlags_NoScrollbar);
        //const auto& drawList = ImGui::GetWindowDrawList();
        //drawList->AddRectFilled(lineNumberStartPos, lineNumberEndPos, IM_COL32(100, 100, 0, 255));

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

void NXTextEditor::Render_MainLayer()
{
    const ImVec2& windowPos = ImGui::GetWindowPos();
    const ImVec2& windowSize = ImGui::GetWindowSize();

    ImGui::SetCursorPosX(m_lineTextStartX);
    ImGui::BeginChild("##text_content", ImVec2(windowSize.x - m_lineTextStartX, windowSize.y), false, ImGuiWindowFlags_HorizontalScrollbar);
    for (int i = 0; i < m_lines.size(); i++)
    {
        const auto& strLine = m_lines[i];
        ImGui::TextUnformatted(strLine.c_str());
    }
    float scrollY_textContent = ImGui::GetScrollY();
    float scrollBarHeight = ImGui::GetScrollMaxY() > 0.0f ? ImGui::GetStyle().ScrollbarSize : 0.0f;
    ImGui::EndChild();

    ImGui::SetCursorPos(ImVec2(0.0f, 0.0f));
    ImGui::BeginChild("##line_number", ImVec2(m_lineNumberWidthWithPaddingX, windowSize.y - scrollBarHeight), false, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    ImGui::SetScrollY(scrollY_textContent);
    Render_LineNumber();
    ImGui::EndChild();
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

void NXTextEditor::HandleInputs()
{
    ImGuiIO& io = ImGui::GetIO();
    {
        bool bClicked = ImGui::IsMouseClicked(ImGuiMouseButton_Left);
        bool bDoubleClicked = ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left);

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
                AddSelection(row, left + 1, row, right);
            }
        }
        else if (bClicked)
        {
            const auto& mousePos = ImGui::GetMousePos();
            const auto& windowPos = ImGui::GetWindowPos();
            const ImVec2 relativeWindowPos(mousePos.x - windowPos.x - m_lineTextStartX, mousePos.y - windowPos.y);

            // 获取在整个文本显示区中的相对位置
            const ImVec2 relativePos(m_scrollX + relativeWindowPos.x, m_scrollY + relativeWindowPos.y);

            // 计算出行列号
            int row = relativePos.y / m_charHeight;
            int col = relativePos.x / m_charWidth;

            // test: 打印对应字符
            //if (col < m_lines[row].size()) std::cout << m_lines[row][col];

            ClearSelection();
            AddSelection(row, col, row, col);
        }

        // 处理鼠标拖拽
        if (ImGui::IsMouseDown(ImGuiMouseButton_Left) && ImGui::IsMouseDragging(ImGuiMouseButton_Left))
        {
        }
    }
}
