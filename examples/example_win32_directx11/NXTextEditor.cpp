﻿#include "NXTextEditor.h"
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

    AddSelection({ 10, 10 }, { 10, 25 });
    AddSelection({ 40, 15 }, { 14, 10 });
}

void NXTextEditor::Render()
{
    if (m_bResetFlickerDt)
    {
        m_flickerDt = ImGui::GetTime();
        m_bResetFlickerDt = false;
    }

    //ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::Begin("TextEditor", (bool*)true);
    if (ImGui::IsWindowFocused()) m_bNeedFocusOnText = true;

    const ImVec2& layerStartCursorPos = ImGui::GetCursorPos();

    ImGui::BeginChild("##main_layer", ImVec2(), false, ImGuiWindowFlags_NoInputs);
    Render_MainLayer();
    ImGui::EndChild();

    ImGui::SetCursorPos(layerStartCursorPos);
    ImGui::BeginChild("##debug_layer", ImVec2(), false, ImGuiWindowFlags_NoInputs);
    Render_DebugLayer();
    ImGui::EndChild();

    ImGui::End();
    //ImGui::PopStyleVar();
}

void NXTextEditor::AddSelection(const Coordinate& A, const Coordinate& B)
{
    SelectionInfo selection(A, B);
	m_selections.push_back(selection);
}

void NXTextEditor::RemoveSelection(const SelectionInfo& removeSelection)
{
    std::erase_if(m_selections, [&removeSelection](const SelectionInfo& selection) { return removeSelection == selection; });
}

void NXTextEditor::ClearSelection()
{
    m_selections.clear();
}

void NXTextEditor::Enter(ImWchar c)
{
}

void NXTextEditor::Backspace()
{
}

void NXTextEditor::Render_MainLayer()
{
    const ImVec2& windowSize = ImGui::GetWindowSize();
    Render_OnMouseInputs();

    ImGui::SetCursorPos(ImVec2(m_lineTextStartX, 0.0f));
    ImGui::BeginChild("##text_content", ImVec2(windowSize.x - m_lineTextStartX, windowSize.y), false, ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_NoNav);
    RenderTexts_OnKeyInputs();
    RenderTexts_OnMouseInputs();
    RenderSelections();
    RenderTexts();
    float scrollY_textContent = ImGui::GetScrollY();
    float scrollBarHeight = ImGui::GetScrollMaxY() > 0.0f ? ImGui::GetStyle().ScrollbarSize : 0.0f;

    if (m_bNeedFocusOnText)
    {
        ImGui::SetKeyboardFocusHere();
        m_bNeedFocusOnText = false;
    }

    ImGui::EndChild();

    ImGui::SetCursorPos(ImVec2(0.0f, 0.0f));
    ImGui::BeginChild("##line_number", ImVec2(m_lineNumberWidthWithPaddingX, windowSize.y - scrollBarHeight), false, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    ImGui::SetScrollY(scrollY_textContent);
    RenderLineNumber();
    ImGui::EndChild();
}

void NXTextEditor::Render_DebugLayer()
{
    ImGui::SetCursorPos(ImVec2(ImGui::GetContentRegionAvail().x - 400.0f, 0.0f));
    ImGui::BeginChild("##debug_selections", ImVec2(350.0f, 0.0f), false, ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoNav);
    for (size_t i = 0; i < m_selections.size(); i++)
    {
        const auto& selection = m_selections[i];
        std::string info = "Selection " + std::to_string(i) + ":(" + std::to_string(selection.L.row) + ", " + std::to_string(selection.L.col) + ") -> (" + std::to_string(selection.R.row) + ", " + std::to_string(selection.R.col) + ")";
        ImGui::Text(info.c_str());
    }

    if (m_bIsSelecting)
    {
        const SelectionInfo selection(m_activeSelectionDown, m_activeSelectionMove);
        std::string info = "Active selection:(" + std::to_string(selection.L.row) + ", " + std::to_string(selection.L.col) + ") -> (" + std::to_string(selection.R.row) + ", " + std::to_string(selection.R.col) + ")";
        ImGui::Text(info.c_str());
    }
    ImGui::EndChild();
}

void NXTextEditor::RenderSelections()
{
    for (const auto& selection : m_selections)
        RenderSelection(selection);

    if (m_bIsSelecting)
    {
        SelectionInfo activeSelection(m_activeSelectionDown, m_activeSelectionMove);
        RenderSelection(activeSelection);
    }
}

void NXTextEditor::RenderTexts()
{
    for (int i = 0; i < m_lines.size(); i++)
    {
        const auto& strLine = m_lines[i];
        ImGui::TextUnformatted(strLine.c_str());
    }
}

void NXTextEditor::RenderLineNumber()
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

void NXTextEditor::RenderSelection(const SelectionInfo& selection)
{
    const ImVec2& windowPos = ImGui::GetWindowPos();
    const auto& drawList = ImGui::GetWindowDrawList();

    float scrollX = ImGui::GetScrollX();
    float scrollY = ImGui::GetScrollY();

    const Coordinate fromPos(selection.L.row, min(selection.L.col, (int)m_lines[selection.L.row].size()));
    const Coordinate toPos(selection.R.row, min(selection.R.col, (int)m_lines[selection.R.row].size()));

    ImVec2 flickerPos;

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

        // 计算闪烁位置
        flickerPos = selection.flickerAtFront ? selectStartPos : ImVec2(selectEndPos.x, selectEndPos.y - m_charHeight);
    }
    else
    {
        // 绘制首行，先确定首行字符长度
        const size_t firstLineLength = m_lines[fromPos.row].length();

        // 行首坐标
        ImVec2 linePos(windowPos.x, windowPos.y + m_charHeight * fromPos.row - scrollY);
        // 绘制所选对象的选中状态矩形
        ImVec2 selectStartPos(linePos.x + fromPos.col * m_charWidth - scrollX, linePos.y);
        ImVec2 selectEndPos(linePos.x + firstLineLength * m_charWidth - scrollX, linePos.y + m_charHeight);
        drawList->AddRectFilled(selectStartPos, selectEndPos, IM_COL32(100, 100, 0, 255));

        // 如果是 B在前，A在后，则在文本开始处闪烁
        if (selection.flickerAtFront) flickerPos = selectStartPos;

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

        // 如果是 A在前，B在后，则在文本末尾处闪烁
        if (!selection.flickerAtFront) flickerPos = ImVec2(selectEndPos.x, selectEndPos.y - m_charHeight);
    }

    // 绘制闪烁条 每秒钟闪烁一次
    if (fmod(ImGui::GetTime() - m_flickerDt, 1.0f) < 0.5f)
        drawList->AddLine(flickerPos, ImVec2(flickerPos.x, flickerPos.y + m_charHeight), IM_COL32(255, 255, 0, 255), 1.0f);
}

void NXTextEditor::SelectionsOverlayCheck(bool bIsAltSelection)
{
    // 检测的 m_selections 的所有元素（下面称为 selection）的覆盖范围是否和当前 { m_activeSelectionDown, m_activeSelectionMove } 重叠
    // 1. 如果 activeSelection 是当前 selection 的父集，则将 selection 删除
    // 2. 如果 activeSelection 是当前 selection 的子集，则将 selection 删除，但将 activeSelectionDown 移至 selection.L
    // 3. 如果 activeSelection 不是 selection 的子集，但和 selection.L 相交，则将 selection 删除；如果是双击事件Check，将 activeSelectionDown 移至 selection.R
    // 4. 如果 activeSelection 不是 selection 的子集，但和 selection.R 相交，则将 selection 删除；如果是双击事件Check，将 activeSelectionMove 移至 selection.L
    // 规则和 VSCode 相似，但也同样有 VSCode 的缺陷...比如 alt 选三块然后在块1中间执行alt重选会破坏掉块2、3。不过影响不大。

    std::erase_if(m_selections, [this, bIsAltSelection](const SelectionInfo& selection)
        {
            SelectionInfo activeSelection(m_activeSelectionDown, m_activeSelectionMove);
            if (activeSelection.Include(selection)) // rule 1. 
                return true;
            else if (selection.Include(activeSelection)) // rule 2.
            {
                m_activeSelectionDown = selection.L;
                return true;
            }
            else if (activeSelection.Include(selection.L)) // rule 3.
            {
                if (bIsAltSelection) m_activeSelectionMove = selection.R;
                return true;
            }
            else if (activeSelection.Include(selection.R)) // rule 4.
            {
                if (bIsAltSelection) m_activeSelectionDown = selection.L;
                return true;
            }
            return false;
        });
}

void NXTextEditor::RenderTexts_OnMouseInputs()
{
    auto& io = ImGui::GetIO();

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
        int row = (int)(relativePos.y / m_charHeight);
        float fCol = relativePos.x / m_charWidth;

        // 手感优化：实际点击位置的 列坐标 超过当前字符的50% 时，认为是下一个字符
        int col = (int)(fCol + 0.5f);

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

            if (!io.KeyAlt) ClearSelection();

            m_bIsSelecting = true;
            m_activeSelectionDown = { row, left + 1 };
            m_activeSelectionMove = { row, right };
            SelectionsOverlayCheck(true);
        }

        m_bResetFlickerDt = true;
    }
    else if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
    {
        // 计算出行列号
        int row = (int)(relativePos.y / m_charHeight);
        float fCol = relativePos.x / m_charWidth;

        // 手感优化：实际点击位置的 列坐标 超过当前字符的50% 时，认为是下一个字符
        int col = (int)(fCol + 0.5f);

        // 约束行列号范围
        row = std::max(0, std::min(row, (int)m_lines.size() - 1));
        col = std::max(0, std::min(col, (int)m_lines[row].size()));

        // test: 打印对应字符
        //if (col < m_lines[row].size()) std::cout << m_lines[row][col];

        if (!io.KeyAlt) ClearSelection();

        m_bIsSelecting = true;
        for (const auto& selection : m_selections)
        {
            const Coordinate& pos = { row, col };
            if (selection.Include(pos))
            {
                m_bIsSelecting = false;
				if (selection == pos) RemoveSelection(selection);
                break;
            }
        }

        if (m_bIsSelecting)
        {
            m_activeSelectionDown = { row, col };
            m_activeSelectionMove = { row, col };
        }

        m_bResetFlickerDt = true;
    }
    else if (ImGui::IsMouseDragging(ImGuiMouseButton_Left))
    {
        // 处理鼠标拖拽
        // 计算出行列号
        int row = (int)(relativePos.y / m_charHeight);
        float fCol = relativePos.x / m_charWidth;

        // 手感优化：实际点击位置的 列坐标 超过当前字符的50% 时，认为是下一个字符
        int col = (int)(fCol + 0.5f);

        // 约束行列号范围
        row = std::max(0, std::min(row, (int)m_lines.size() - 1));
        col = std::max(0, std::min(col, (int)m_lines[row].size()));

        m_bIsSelecting = true;
		m_activeSelectionMove = { row, col };
        SelectionsOverlayCheck(false);

        m_bResetFlickerDt = true;
    }
}

void NXTextEditor::Render_OnMouseInputs()
{
    if (m_bIsSelecting && ImGui::IsMouseReleased(ImGuiMouseButton_Left))
    {
        m_bIsSelecting = false;
        AddSelection(m_activeSelectionDown, m_activeSelectionMove);
    }
}

void NXTextEditor::RenderTexts_OnKeyInputs()
{
    ImGuiIO& io = ImGui::GetIO();
	bool bAlt = io.KeyAlt;
	bool bShift = io.KeyShift;
	bool bCtrl = io.KeyCtrl;
	bool bModify = bAlt || bShift || bCtrl;

    if (ImGui::IsWindowFocused())
    {
        io.WantCaptureKeyboard = true;
        io.WantTextInput = true;

        if (!bModify && ImGui::IsKeyPressed(ImGuiKey_UpArrow))
        {
            MoveUp();
            m_bResetFlickerDt = true;
        }

        else if (!bModify && ImGui::IsKeyPressed(ImGuiKey_DownArrow))
        {
            MoveDown();
            m_bResetFlickerDt = true;
        }

        else if (!bModify && ImGui::IsKeyPressed(ImGuiKey_LeftArrow))
        {
            MoveLeft();
            m_bResetFlickerDt = true;
        }

        else if (!bModify && ImGui::IsKeyPressed(ImGuiKey_RightArrow))
        {
            MoveRight();
            m_bResetFlickerDt = true;
        }

        if (!io.InputQueueCharacters.empty())
        {
            for (const auto& c : io.InputQueueCharacters)
            {
                if (c != 0 && (c == '\n' || c >= 32))
                {
                    Enter(c);
                    m_bResetFlickerDt = true;
                }
            }

            io.InputQueueCharacters.resize(0);
        }
    }
}

void NXTextEditor::MoveUp()
{
    for (auto& sel : m_selections)
    {
        auto& pos = sel.flickerAtFront ? sel.L : sel.R;
        if (pos.row > 0) pos.row--;
        sel.flickerAtFront ? sel.R = pos : sel.L = pos;
    }
}

void NXTextEditor::MoveDown()
{
    for (auto& sel : m_selections)
    {
        auto& pos = sel.flickerAtFront ? sel.L : sel.R;
        if (pos.row < m_lines.size() - 1) pos.row++;
        sel.flickerAtFront ? sel.R = pos : sel.L = pos;
    }
}

void NXTextEditor::MoveLeft()
{
    for (auto& sel : m_selections)
    {
        auto& pos = sel.flickerAtFront ? sel.L : sel.R;
        pos.col = min(pos.col, (int)m_lines[pos.row].size());
        if (pos.col > 0)
            pos.col--;
        else if (pos.row > 0)
        {
            pos.row--;
            pos.col = (int)m_lines[pos.row].size();
        }
        sel.flickerAtFront ? sel.R = pos : sel.L = pos;
    }
}

void NXTextEditor::MoveRight()
{
    for (auto& sel : m_selections)
    {
        auto& pos = sel.flickerAtFront ? sel.L : sel.R;
        pos.col = min(pos.col, (int)m_lines[pos.row].size());
        if (pos.col < m_lines[pos.row].size()) pos.col++;
        else if (pos.row < m_lines.size() - 1)
        {
            pos.row++;
            pos.col = 0;
        }
        sel.flickerAtFront ? sel.R = pos : sel.L = pos;
    }
}
