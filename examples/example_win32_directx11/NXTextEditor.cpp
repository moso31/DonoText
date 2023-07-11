﻿#include "NXTextEditor.h"
#include <fstream>
#include <iostream>
#include <cctype>

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
    // get single char size of font
    auto font = ImGui::GetFont();
    const ImVec2 fontSize = font->CalcTextSizeA(font->FontSize, FLT_MAX, -1.0f, " ");

    // 2023.7.4 仅支持等宽字体！其它字体感觉略有点吃性能，且没什么必要。
    // 预存储单个字符的 xy像素大小
    m_charWidth = fontSize.x;
    m_charHeight = ImGui::GetTextLineHeightWithSpacing(); // fontSize.y + style.ItemSpacing;

    // 记录行号文本能达到的最大宽度
    m_lineNumberWidth = m_charWidth * std::to_string(m_lines.size()).length();

    m_lineNumberWidthWithPaddingX = m_lineNumberWidth + m_lineNumberPaddingX * 2.0f;

    // 行号矩形 - 文本 之间留一个4px的空
    float paddingX = 4.0f;
    m_lineTextStartX = m_lineNumberWidthWithPaddingX + paddingX;
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
    ImGui::Text("Disable Render_DebugLayer() method to hide me!");

    for (size_t i = 0; i < m_selections.size(); i++)
    {
        const auto& selection = m_selections[i];
        std::string info = "Selection " + std::to_string(i) + ":(" + std::to_string(selection.L.row) + ", " + std::to_string(selection.L.col) + ") " + 
            std::string(selection.flickerAtFront ? "<-" : "->") +
            " (" + std::to_string(selection.R.row) + ", " + std::to_string(selection.R.col) + ")";
        ImGui::Text(info.c_str());
    }

    if (m_bIsSelecting)
    {
        const SelectionInfo selection(m_activeSelectionDown, m_activeSelectionMove);
        std::string info = "Active selection:(" + std::to_string(selection.L.row) + ", " + std::to_string(selection.L.col) + ") " +
            std::string(selection.flickerAtFront ? "<-" : "->") +
            " (" + std::to_string(selection.R.row) + ", " + std::to_string(selection.R.col) + ")";
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
        std::string strLineNumber = std::to_string(i + 1);
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

    const Coordinate fromPos(selection.L.row, selection.L.col);
    const Coordinate toPos(selection.R.row, selection.R.col);

    ImVec2 flickerPos;

    // 判断 A B 是否在同一行
    const bool bSingleLine = fromPos.row == toPos.row;

    if (bSingleLine)
    {
        int linePosL = std::min(fromPos.col, (int)m_lines[fromPos.row].length());
        int linePosR = std::min(toPos.col, (int)m_lines[toPos.row].length());

        // 行首坐标
        ImVec2 linePos(windowPos.x, windowPos.y + m_charHeight * fromPos.row - scrollY);

        // 绘制所选对象的选中状态矩形
        ImVec2 selectStartPos(linePos.x + linePosL * m_charWidth - scrollX, linePos.y);
        ImVec2 selectEndPos(linePos.x + linePosR * m_charWidth - scrollX, linePos.y + m_charHeight);
        drawList->AddRectFilled(selectStartPos, selectEndPos, IM_COL32(100, 100, 0, 255));

        // 计算闪烁位置
        flickerPos = selection.flickerAtFront ? selectStartPos : ImVec2(selectEndPos.x, selectEndPos.y - m_charHeight);
    }
    else
    {
        // 多行文本除最后一行，其它行的选中矩形都向右增加一个字符长度
        float enterCharOffset = m_charWidth;

        // 绘制首行，先确定首行字符坐标
        int linePosR = (int)m_lines[fromPos.row].length();
        int linePosL = std::min(fromPos.col, linePosR);

        // 行首坐标
        ImVec2 linePos(windowPos.x, windowPos.y + m_charHeight * fromPos.row - scrollY);
        // 绘制所选对象的选中状态矩形
        ImVec2 selectStartPos(linePos.x + linePosL * m_charWidth - scrollX, linePos.y);
        ImVec2 selectEndPos(linePos.x + linePosR * m_charWidth - scrollX, linePos.y + m_charHeight);
        drawList->AddRectFilled(selectStartPos, ImVec2(selectEndPos.x + enterCharOffset, selectEndPos.y), IM_COL32(100, 100, 0, 255));

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
            drawList->AddRectFilled(selectStartPos, ImVec2(selectEndPos.x + enterCharOffset, selectEndPos.y), IM_COL32(100, 100, 0, 255));
        }

        // 绘制尾行，确定尾行字符坐标
        linePosL = 0;
        linePosR = std::min(toPos.col, (int)m_lines[toPos.row].length());

        // 行首坐标
        linePos = ImVec2(windowPos.x, windowPos.y + m_charHeight * toPos.row - scrollY);
        // 绘制所选对象的选中状态矩形
        selectStartPos = ImVec2(linePos.x - linePosL * m_charWidth - scrollX, linePos.y);
        selectEndPos = ImVec2(linePos.x + linePosR * m_charWidth - scrollX, linePos.y + m_charHeight);
        drawList->AddRectFilled(selectStartPos, selectEndPos, IM_COL32(100, 100, 0, 255));

        // 如果是 A在前，B在后，则在文本末尾处闪烁
        if (!selection.flickerAtFront) flickerPos = ImVec2(selectEndPos.x, selectEndPos.y - m_charHeight);
    }

    // 绘制闪烁条 每秒钟闪烁一次
    if (fmod(ImGui::GetTime() - m_flickerDt, 1.0f) < 0.5f)
        drawList->AddLine(flickerPos, ImVec2(flickerPos.x, flickerPos.y + m_charHeight), IM_COL32(255, 255, 0, 255), 1.0f);
}

void NXTextEditor::SelectionsOverlayCheckForMouseEvent(bool bIsDoubleClick)
{
    // 检测的 m_selections 的所有元素（下面称为 selection）的覆盖范围是否和当前 { m_activeSelectionDown, m_activeSelectionMove } 重叠
    // 1. 如果 activeSelection 是当前 selection 的父集，则将 selection 删除
    // 2. 如果 activeSelection 是当前 selection 的子集，则将 selection 删除，但将 activeSelectionDown 移至 selection.L
    // 3. 如果 activeSelection 不是 selection 的子集，但和 selection.L 相交，则将 selection 删除；如果是双击事件Check，将 activeSelectionDown 移至 selection.R
    // 4. 如果 activeSelection 不是 selection 的子集，但和 selection.R 相交，则将 selection 删除；如果是双击事件Check，将 activeSelectionMove 移至 selection.L

    std::erase_if(m_selections, [this, bIsDoubleClick](const SelectionInfo& selection)
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
                if (bIsDoubleClick) m_activeSelectionMove = selection.R;
                return true;
            }
            else if (activeSelection.Include(selection.R)) // rule 4.
            {
                if (bIsDoubleClick) m_activeSelectionDown = selection.L;
                return true;
            }
            return false;
        });
}

void NXTextEditor::SelectionsOverlayCheckForKeyEvent(bool bFlickerAtFront)
{
    // 检测键盘输入事件导致Selection发生变化以后是否相互重叠，
    // 如果两个 selection 重叠，合并成一个。重叠以后的 新selection 是否 flickerAtFront 由键盘事件的类型决定。

    m_overlaySelectCheck.clear();
    m_overlaySelectCheck.reserve(m_selections.size() * 2);
    for (const auto& selection: m_selections)
    {
        m_overlaySelectCheck.push_back({ selection.L, true, selection.flickerAtFront });
        m_overlaySelectCheck.push_back({ selection.R, false, selection.flickerAtFront });

        // 排序前将右坐标后移一格，确保将 sel1.R=sel2.L 这种坐标相同但不相交的情况也视作重叠。
        m_overlaySelectCheck.back().value.col++;
    }
    std::sort(m_overlaySelectCheck.begin(), m_overlaySelectCheck.end(), [](const SignedCoordinate& a, const SignedCoordinate& b) { return a.value < b.value; });

    // 排完序需要再移动回来
    for (auto& overlayCheck : m_overlaySelectCheck)
        if (!overlayCheck.isLeft) overlayCheck.value.col--;

    Coordinate left, right;
    int leftSignCount = 0; // 遇左+1，遇右-1。
    bool overlayed = false; // 当 leftSignCount>1 时，说明有重叠产生，使用此值记录

    // 检测重叠，如果有重叠的selection，去掉
    m_selections.clear();
    for (auto& coord : m_overlaySelectCheck)
    {
        if (coord.isLeft)
        {
            if (leftSignCount == 0)
            {
                left = coord.value;
            }
            else if (leftSignCount > 0) overlayed = true; // 检测是否出现了重叠
            leftSignCount++;
        }
        else
        {
            leftSignCount--;
            if (leftSignCount == 0)
            {
                right = coord.value;
                if (overlayed)
                {
                    // 如果出现重叠，基于当前键盘事件判断 flickerAtFront
                    m_selections.push_back(bFlickerAtFront ? SelectionInfo(right, left) : SelectionInfo(left, right));
                }
                else
                {
                    // 如果没有出现重叠，保留之前的 flickerAtFront
                    m_selections.push_back(coord.flickerAtFront ? SelectionInfo(right, left) : SelectionInfo(left, right));
                }
                overlayed = false;
            }
        }
    }
}

void NXTextEditor::ScrollCheckForKeyEvent()
{
    // 此方法负责当屏幕外有 selection 选取变化时，跳转到该位置。
    // 2023.7.11 暂跳转到 m_selections.back() 所在的位置。
    // 屏幕内存在选区的情况下，此策略有很明显的手感问题，有待优化。
    const auto& lastSelection = m_selections.back();
    int lastRow = lastSelection.flickerAtFront ? lastSelection.L.row : lastSelection.R.row;
    int lastCol = lastSelection.flickerAtFront ? lastSelection.L.col : lastSelection.R.col;

    // 如果超出窗口边界，scrollY
    float scrollY = ImGui::GetScrollY();
    float scrollMaxY = ImGui::GetScrollMaxY();
    float contentAreaHeight = ImGui::GetContentRegionAvail().y;
    float newSelectHeight = (float)lastRow * m_charHeight;
    if (newSelectHeight < scrollY)
    {
        ImGui::SetScrollY(newSelectHeight);
    }
    else if (newSelectHeight > scrollY + contentAreaHeight - m_charHeight)
    {
        ImGui::SetScrollY(std::min(newSelectHeight - contentAreaHeight + m_charHeight, scrollMaxY));
    }

    // 如果超出窗口边界，scrollX
    float scrollX = ImGui::GetScrollX();
    float scrollMaxX = ImGui::GetScrollMaxX();
    float contentAreaWidth = ImGui::GetContentRegionAvail().x;
    float newSelectWidth = (float)lastCol * m_charWidth;
    if (newSelectWidth < scrollX)
    {
        ImGui::SetScrollX(newSelectWidth);
    }
    else if (newSelectWidth > scrollX + contentAreaWidth)
    {
        ImGui::SetScrollX(std::min(newSelectWidth - contentAreaWidth + 2 * m_charWidth, scrollMaxX));
    }
}

void NXTextEditor::RenderTexts_OnMouseInputs()
{
    if (!ImGui::IsWindowFocused())
        return;

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

    if (isMouseInContentArea && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
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
            SelectionsOverlayCheckForMouseEvent(true);
        }

        m_bResetFlickerDt = true;
    }
    else if (isMouseInContentArea && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
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
    else if (m_bIsSelecting && ImGui::IsMouseDragging(ImGuiMouseButton_Left))
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

		m_activeSelectionMove = { row, col };
        SelectionsOverlayCheckForMouseEvent(false);

        m_bResetFlickerDt = true;

        // 超出当前显示范围时 scrollX/Y
        const ImVec2 dragSpeed(m_charWidth * 4.0f, m_charHeight * 2.0f);
        const auto& contentArea(ImGui::GetContentRegionAvail());
        const ImVec2 scrollMax(ImGui::GetScrollMaxX(), ImGui::GetScrollMaxY());
        float scrollBarWidth = scrollMax.x > 0.0f ? ImGui::GetStyle().ScrollbarSize : 0.0f;
        float scrollBarHeight = scrollMax.y > 0.0f ? ImGui::GetStyle().ScrollbarSize : 0.0f;

        if (relativeWindowPos.x < 0)
            ImGui::SetScrollX(std::max(scrollX - dragSpeed.x, 0.0f));
        else if (relativeWindowPos.x > contentArea.x + scrollBarWidth)
        {
            // 当 scrollX 右移时，不准超过当前鼠标所在行的最大文本长度 + 50.0 像素
            float scrollXLimit = m_lines[row].size() * m_charWidth - contentArea.x + 50.0f;
            scrollXLimit = std::min(scrollXLimit, scrollMax.x);
            ImGui::SetScrollX(std::min(scrollX + dragSpeed.x, scrollXLimit));
        }

        if (relativeWindowPos.y < 0)
            ImGui::SetScrollY(std::max(scrollY - dragSpeed.y, 0.0f));
        else if (relativeWindowPos.y > contentArea.y + scrollBarHeight)
            ImGui::SetScrollY(std::min(scrollY + dragSpeed.y, scrollMax.y));
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

        bool bKeyUpPressed = ImGui::IsKeyPressed(ImGuiKey_UpArrow);
        bool bKeyDownPressed = ImGui::IsKeyPressed(ImGuiKey_DownArrow);
        bool bKeyLeftPressed = ImGui::IsKeyPressed(ImGuiKey_LeftArrow);
        bool bKeyRightPressed = ImGui::IsKeyPressed(ImGuiKey_RightArrow);
        bool bKeyHomePressed = ImGui::IsKeyPressed(ImGuiKey_Home);
        bool bKeyEndPressed = ImGui::IsKeyPressed(ImGuiKey_End);
        bool bKeyPageUpPressed = ImGui::IsKeyPressed(ImGuiKey_PageUp);
        bool bKeyPageDownPressed = ImGui::IsKeyPressed(ImGuiKey_PageDown);

        bool bCtrlHomePressed = bCtrl && bKeyHomePressed;
        bool bCtrlEndPressed = bCtrl && bKeyEndPressed;
        if (!bAlt && (bKeyUpPressed || bKeyPageUpPressed || bCtrlHomePressed))
        {
            MoveUp(bShift, bKeyPageUpPressed, bCtrlHomePressed);
            m_bResetFlickerDt = true;
        }

        else if (!bAlt && (bKeyDownPressed || bKeyPageDownPressed || bCtrlEndPressed))
        {
            MoveDown(bShift, bKeyPageDownPressed, bCtrlEndPressed);
            m_bResetFlickerDt = true;
        }

        else if (!bAlt && (bKeyLeftPressed || bKeyHomePressed))
        {
            MoveLeft(bShift, bCtrl, bKeyHomePressed);
            m_bResetFlickerDt = true;
        }

        else if (!bAlt && (bKeyRightPressed || bKeyEndPressed))
        {
            MoveRight(bShift, bCtrl, bKeyEndPressed);
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

void NXTextEditor::MoveUp(bool bShift, bool bPageUp, bool bCtrlHome)
{
    for (auto& sel : m_selections)
    {
        auto& pos = sel.flickerAtFront ? sel.L : sel.R;
        bCtrlHome ? pos.row = -1 : bPageUp ? pos.row -= 20 : pos.row--;
        if (pos.row < 0)
        {
            pos.row = 0;
            pos.col = 0;
        }

        if (!bShift)
            sel.flickerAtFront ? sel.R = pos : sel.L = pos;
        else
        {
            if (sel.R < sel.L)
            {
                std::swap(sel.L, sel.R);
                sel.flickerAtFront = true;
            }

            SelectionsOverlayCheckForKeyEvent(true);
        }
    }

    ScrollCheckForKeyEvent();
}

void NXTextEditor::MoveDown(bool bShift, bool bPageDown, bool bCtrlEnd)
{
    int maxRow = 0;
    int maxCol = 0;
    for (auto& sel : m_selections)
    {
        auto& pos = sel.flickerAtFront ? sel.L : sel.R;
        bCtrlEnd ? pos.row = (int)m_lines.size() : bPageDown ? pos.row += 20 : pos.row++;
        if (pos.row >= m_lines.size())
        {
            pos.row = (int)m_lines.size() - 1;
            pos.col = (int)m_lines[pos.row].size();
        }

        maxRow = std::max(maxRow, pos.row);
        maxCol = std::max(maxCol, pos.col);

        if (!bShift)
            sel.flickerAtFront ? sel.R = pos : sel.L = pos;
        else
        {
            if (sel.L > sel.R)
            {
                std::swap(sel.L, sel.R);
                sel.flickerAtFront = false;
            }

            SelectionsOverlayCheckForKeyEvent(false);
        }
    }

    // 如果超出窗口边界，scrollY
    float scrollY = ImGui::GetScrollY();
    float scrollMaxY = ImGui::GetScrollMaxY();
    float contentAreaHeight = ImGui::GetContentRegionAvail().y;
    float newSelectHeight = (float)maxRow * m_charHeight;
    if (newSelectHeight < scrollY)
    {
        ImGui::SetScrollY(newSelectHeight);
    }
    else if (newSelectHeight > scrollY + contentAreaHeight - m_charHeight)
    {
        ImGui::SetScrollY(std::min(newSelectHeight - contentAreaHeight + m_charHeight, scrollMaxY));
    }

    ScrollCheckForKeyEvent();
}

void NXTextEditor::MoveLeft(bool bShift, bool bCtrl, bool bHome)
{
    int maxRow = 0;
    int maxCol = 0;
    for (auto& sel : m_selections)
    {
        auto& pos = sel.flickerAtFront ? sel.L : sel.R;
        pos.col = std::min(pos.col, (int)m_lines[pos.row].size());
        if (bHome) pos.col = 0;
        else if (pos.col > 0)
        {
            pos.col--;
            if (bCtrl)
            {
                while (pos.col > 0 && !IsVariableChar(m_lines[pos.row][pos.col])) pos.col--;
                if (pos.col > 0)
                {
                    while (pos.col > 0 && IsVariableChar(m_lines[pos.row][pos.col])) pos.col--;
                    pos.col++;
                }
            }
        }
        else if (pos.row > 0)
        {
            pos.row--;
            pos.col = (int)m_lines[pos.row].size();
        }

        maxRow = std::max(maxRow, pos.row);
        maxCol = std::max(maxCol, pos.col);

        if (!bShift)
            sel.flickerAtFront ? sel.R = pos : sel.L = pos;
        else
        {
            if (sel.R < sel.L)
            {
                std::swap(sel.L, sel.R);
                sel.flickerAtFront = true;
            }

            SelectionsOverlayCheckForKeyEvent(true);
        }
    }

    // 如果超出窗口边界，scrollY
    float scrollY = ImGui::GetScrollY();
    float scrollMaxY = ImGui::GetScrollMaxY();
    float contentAreaHeight = ImGui::GetContentRegionAvail().y;
    float newSelectHeight = (float)maxRow * m_charHeight;
    if (newSelectHeight < scrollY)
    {
        ImGui::SetScrollY(newSelectHeight);
    }
    else if (newSelectHeight > scrollY + contentAreaHeight - m_charHeight)
    {
        ImGui::SetScrollY(std::min(newSelectHeight - contentAreaHeight + m_charHeight, scrollMaxY));
    }

    ScrollCheckForKeyEvent();
}

void NXTextEditor::MoveRight(bool bShift, bool bCtrl, bool bEnd)
{
    int maxRow = 0;
    int maxCol = 0;
    for (auto& sel : m_selections)
    {
        auto& pos = sel.flickerAtFront ? sel.L : sel.R;
        pos.col = std::min(pos.col, (int)m_lines[pos.row].size());
        if (bEnd) pos.col = (int)m_lines[pos.row].size();
        else if (pos.col < m_lines[pos.row].size())
        {
            pos.col++;
            if (bCtrl)
            {
                while (pos.col < m_lines[pos.row].size() && !IsVariableChar(m_lines[pos.row][pos.col])) pos.col++;
                if (pos.col < m_lines[pos.row].size())
                {
                    while (pos.col < m_lines[pos.row].size() && IsVariableChar(m_lines[pos.row][pos.col])) pos.col++;
                }
            }
        }
        else if (pos.row < m_lines.size() - 1)
        {
            pos.row++;
            pos.col = 0;
        }

        maxRow = std::max(maxRow, pos.row);
        maxCol = std::max(maxCol, pos.col);

        if (!bShift)
            sel.flickerAtFront ? sel.R = pos : sel.L = pos;
        else
        {
            if (sel.L > sel.R)
            {
                std::swap(sel.L, sel.R);
                sel.flickerAtFront = false;
            }

            SelectionsOverlayCheckForKeyEvent(false);
        }
    }

    ScrollCheckForKeyEvent();
}

bool NXTextEditor::IsVariableChar(const char& ch)
{
    // ctrl+left/right 可以跳过的字符：
    return std::isalnum(ch) || ch == '_';
}
