﻿#include "NXTextEditor.h"
#include <fstream>
#include <iostream>
#include <cctype>
#include <map>

NXTextEditor::NXTextEditor(ImFont* pFont) :
    m_pFont(pFont)
{
	// 逐行读取某个文件的文本信息 
	//std::ifstream file("..\\..\\imgui_demo.cpp");
	//std::ifstream file("..\\..\\license.txt");
	std::ifstream file("..\\..\\a.txt");
    //std::ifstream file("D:\\Users\\Administrator\\Source\\Repos\\DonoText\\imgui_demo.cpp");

	// 逐行读取文件内容到 m_lines 
	TextString line;
	while (std::getline(file, line))
	{
        line.formatArray.clear();
        HighLightSyntax(line);
		m_lines.push_back(line);
	}
}

void NXTextEditor::Init()
{
    // get single char size of font
    const ImVec2 fontSize = m_pFont->CalcTextSizeA(m_pFont->FontSize, FLT_MAX, -1.0f, " ");

    // 2023.7.4 仅支持等宽字体！其它字体感觉略有点吃性能，且没什么必要。

    // 预存储单个字符的 xy像素大小
    m_charWidth = fontSize.x;
    m_charHeight = fontSize.y + ImGui::GetStyle().ItemSpacing.y;

    // 行号至少有两位的宽度
    m_maxLineNumber = 99;
    CalcLineNumberRectWidth();
}

void NXTextEditor::Render()
{
    ImGui::PushFont(m_pFont);
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 4.0f));

    if (m_bResetFlickerDt)
    {
        m_flickerDt = ImGui::GetTime();
        m_bResetFlickerDt = false;
    }

    size_t newLineNumber = std::max(m_maxLineNumber, m_lines.size());
    if (m_maxLineNumber < newLineNumber)
    {
        m_maxLineNumber = newLineNumber;
        CalcLineNumberRectWidth(); // 行号超出渲染矩阵范围时，重新计算渲染矩阵的宽度
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

    ImGui::PopStyleVar();
    ImGui::PopFont();
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

void NXTextEditor::Enter(const std::vector<std::vector<std::string>>& strArray)
{
    // 按行列号顺序排序
    std::sort(m_selections.begin(), m_selections.end(), [](const SelectionInfo& a, const SelectionInfo& b) { return a.R < b.R; });

    // 从后往前挨个处理，复杂度O(selection^2)
    // 每处理一个 selection，都需要补偿计算之前算过的所有 selection 的位置
    for (int i = (int)m_selections.size() - 1; i >= 0; i--)
    {
        auto& selection = m_selections[i];
        const auto& L = selection.L;
        const auto& R = selection.R;

        // 若有选区，先清空
        if (L != R) 
        {
            // 有选区：删除选区中的所有内容
            if (L.row == R.row) // 单行
            {
                auto& line = m_lines[L.row];
                line.erase(line.begin() + L.col, line.begin() + R.col);

                // 补偿计算
                for (int j = i + 1; j < m_selections.size(); j++)
                {
                    auto& sel = m_selections[j];
                    int shiftLength = CalcSelectionLength(selection);
                    if (sel.L.row != L.row) break; // 单行时只需处理同行内后面的文本
                    sel.L.col -= shiftLength;
                    sel.R.col -= shiftLength;
                }
            }
            else // 多行
            {
                // 删除左侧行的右侧部分
                auto& lineL = m_lines[L.row];
                int startErasePos = std::min(L.col, (int)lineL.size());
                lineL.erase(lineL.begin() + startErasePos, lineL.end());

                // 删除右侧行的左侧部分
                auto& lineR = m_lines[R.row];
                int endErasePos = std::min(R.col, (int)lineR.size());
                lineR.erase(lineR.begin(), lineR.begin() + endErasePos);

                // 将右侧行的内容合并到左侧行
                lineL.append(lineR);

                // 删除中间行
                m_lines.erase(std::max(m_lines.begin(), m_lines.begin() + L.row + 1), std::min(m_lines.begin() + R.row + 1, m_lines.end()));

                // 补偿计算
                bool bSameLine = true;
                for (int j = i + 1; j < m_selections.size(); j++)
                {
                    auto& sel = m_selections[j];
                    if (sel.L.row != R.row) bSameLine = false;
                    if (bSameLine)
                    {
                        // 后续选区如果在同一行
                        int shiftLength = R.col - L.col;
                        sel.L.row = L.row;
                        sel.L.col -= shiftLength;
                        sel.R = sel.L;
                    }
                    else
                    {
                        // 后续选区如果不在同一行
                        int shiftLength = R.row - L.row;
                        sel.L.row -= shiftLength;
                        sel.R.row -= shiftLength;
                    }
                }
            }

            // 更新光标位置
            selection.L = selection.R = Coordinate(L.row, L.col);
        }

        // 清空完成，开始插入文本...
        auto& line = m_lines[L.row];
        int allLineIdx = 0;
        std::string strPart2;
        for (int strIdx = 0; strIdx < strArray.size(); strIdx++)
        {
            const auto& str = strArray[strIdx];
            for (int lineIdx = 0; lineIdx < str.size(); lineIdx++, allLineIdx++)
            {
                const auto& strLine = str[lineIdx];
                if (allLineIdx == 0)
                {
                    int cutIdx = std::min(L.col, (int)line.size());
                    // 如果是第一段的第一行，在光标处截断原始字符串，在前半段后面插入新文本。同时保留后半段。
                    std::string strPart1 = line.substr(0, cutIdx);
                    strPart2 = line.substr(cutIdx);
                    line = strPart1 + strLine;
                }
                else
                {
                    // 其他段全部直接插入整行
                    m_lines.insert(m_lines.begin() + L.row + allLineIdx, strLine);
                }

                // 如果是最后一段的最后一行。将之前保留的后半段续上。
                if (strIdx == strArray.size() - 1 && lineIdx == str.size() - 1)
                {
                    m_lines[L.row + allLineIdx] += strPart2;
                }
            }
        }

        // 更新 selection 的位置
        if (allLineIdx <= 0) {} // 什么都不做
        else if (allLineIdx == 1) // 输入只有一行
        {
            int shiftLength = (int)strArray[0][0].length();
            selection.L.col += shiftLength;
            selection.R = selection.L;

            // 补偿计算
            for (int j = i + 1; j < m_selections.size(); j++)
            {
                auto& sel = m_selections[j];
                if (sel.L.row != L.row) break; // 单行时只需处理同行内的 selection
                sel.L.col += shiftLength;
                sel.R.col += shiftLength;
            }
        }
        else // 输入有多行
        {
            int shiftLength = (int)strArray.back().back().length();
            selection.L.row += allLineIdx - 1;
            selection.L.col = shiftLength;
            selection.R = selection.L;

            // 补偿计算
            for (int j = i + 1; j < m_selections.size(); j++)
            {
                auto& sel = m_selections[j];
                sel.L.row += allLineIdx - 1;
                sel.L.col = shiftLength;
                sel.R = sel.L;
            }
        }
    }

    SelectionsOverlayCheckForKeyEvent(false);
    ScrollCheckForKeyEvent();
}

void NXTextEditor::Backspace(bool bDelete, bool bCtrl)
{
    std::sort(m_selections.begin(), m_selections.end(), [](const SelectionInfo& a, const SelectionInfo& b) { return a.R < b.R; });

    // 按行列号从后往前挨个处理，复杂度O(selection^2)
    // 每处理一个 selection，都需要补偿计算之前算过的所有 selection 的位置
    for (int i = (int)m_selections.size() - 1; i >= 0; i--)
    {
        auto& selection = m_selections[i];
        const auto& L = selection.L;
        const auto& R = selection.R;

        if (L == R)
        {
            // 无选区：删除上一个字符，光标退一格
            auto& line = m_lines[L.row];

            bool bNeedCombineLastLine = (L.row > 0 && L.col == 0 && !bDelete); // 需要和上一行合并：位于列首，且按了backspace；
            bool bNeedCombineNextLine = (L.row < m_lines.size() - 1 && L.col == line.size() && bDelete); // 需要和下一行合并：位于列尾，且按了delete；
            bool bNeedCombineLines = bNeedCombineLastLine || bNeedCombineNextLine;

            if (!bNeedCombineLines)
            {
                int eraseSize = 1; // 按字符删除
                if (bCtrl) // 按单词删除
                {
                    int pos = L.col;
                    if (!bDelete) // ctrl+backspace
                    {
                        while (pos > 0 && line[pos - 1] == ' ') pos--;
                        while (pos > 0 && line[pos - 1] != ' ') pos--;
                        eraseSize = std::max(0, L.col - pos);
                    }
                    else // ctrl+delete
                    {
                        while (pos < line.size() && line[pos] == ' ') pos++;
                        while (pos < line.size() && line[pos] != ' ') pos++;
                        eraseSize = std::max(0, pos - L.col);
                    }
                }

                if (!bDelete)
                {
                    int erasePos = std::min(L.col - eraseSize, (int)line.size());
                    line.erase(line.begin() + erasePos, line.begin() + L.col);
                    selection.L = selection.R = Coordinate(L.row, erasePos);
                }
                else
                {
                    int erasePos = std::min(L.col + eraseSize, (int)line.size());
                    line.erase(line.begin() + L.col, line.begin() + erasePos);
                }

                // 补偿计算
                for (int j = i + 1; j < m_selections.size(); j++)
                {
                    auto& sel = m_selections[j];
                    if (sel.L.row != L.row) break; // 单行时只需处理同行内后面的文本
                    sel.L.col -= eraseSize;
                    sel.R.col -= eraseSize;
                }
            }
            else // 跨行
            {
                if (bNeedCombineLastLine)
                {
                    auto& lastLine = m_lines[L.row - 1];
                    int lastLength = (int)lastLine.length();
                    lastLine.append(line);
                    m_lines.erase(m_lines.begin() + L.row);

                    selection.L = selection.R = Coordinate(L.row - 1, lastLength);
                }
                else if (bNeedCombineNextLine)
                {
                    auto& nextLine = m_lines[L.row + 1];
                    line.append(nextLine);
                    m_lines.erase(m_lines.begin() + L.row + 1);
                }

                // 补偿计算
                for (int j = i + 1; j < m_selections.size(); j++)
                {
                    auto& sel = m_selections[j];
                    sel.L.row--;
                    if (sel.L.row == L.row) sel.L.col += L.col; // 当在列尾 delete 时，特殊处理
                    sel.R = sel.L;
                }
            }
        }
        else
        {
            // 有选区：删除选区中的所有内容
            if (L.row == R.row) // 单行
            {
                auto& line = m_lines[L.row];
                line.erase(line.begin() + L.col, line.begin() + R.col);

                // 补偿计算
                for (int j = i + 1; j < m_selections.size(); j++)
                {
                    auto& sel = m_selections[j];
                    int shiftLength = CalcSelectionLength(selection);
                    if (sel.L.row != L.row) break; // 单行时只需处理同行内后面的文本
                    sel.L.col -= shiftLength;
                    sel.R.col -= shiftLength;
                }
            }
            else // 多行
            {
                // 删除左侧行的右侧部分
                auto& lineL = m_lines[L.row];
                int startErasePos = std::min(L.col, (int)lineL.size());
                lineL.erase(lineL.begin() + startErasePos, lineL.end());

                // 删除右侧行的左侧部分
                auto& lineR = m_lines[R.row];
                int endErasePos = std::min(R.col, (int)lineR.size());
                lineR.erase(lineR.begin(), lineR.begin() + endErasePos);

                // 将右侧行的内容合并到左侧行
                lineL.append(lineR);

                // 删除中间行
                m_lines.erase(std::max(m_lines.begin(), m_lines.begin() + L.row + 1), std::min(m_lines.begin() + R.row + 1, m_lines.end()));

                // 补偿计算
                bool bSameLine = true;
                for (int j = i + 1; j < m_selections.size(); j++)
                {
                    auto& sel = m_selections[j];
                    if (sel.L.row != R.row) bSameLine = false;
                    if (bSameLine)
                    {
                        // 后续选区如果在同一行
                        int shiftLength = R.col - L.col;
                        sel.L.row = L.row;
                        sel.L.col -= shiftLength;
                        sel.R = sel.L;
                    }
                    else
                    {
                        // 后续选区如果不在同一行
                        int shiftLength = R.row - L.row;
                        sel.L.row -= shiftLength;
                        sel.R.row -= shiftLength;
                    }
                }
            }

            // 更新光标位置
            selection.L = selection.R = Coordinate(L.row, L.col);
        }
    }

    SelectionsOverlayCheckForKeyEvent(false);
    ScrollCheckForKeyEvent();
}

void NXTextEditor::Escape()
{
    // 按 Esc 后，仅保留最后的 Selection，其它的清除。
    auto lastSel = m_selections.back();
    m_selections.assign(1, lastSel);
}

void NXTextEditor::Copy()
{
    std::vector<std::string> copyLines;

    // Copy策略：遍历所有 selections，
    // 1. 如果selection是个单选光标，copy光标所在的一整行。
    // 2. 如果selection是个选区，copy整个选区；
    for (const auto& selection : m_selections)
    {
        const auto& L = selection.L;
        const auto& R = selection.R;
        if (L == R) // rule 1.
        {
            copyLines.push_back("");
            copyLines.push_back(m_lines[L.row]);
        }
        else // rule 2
        {
            if (L.row == R.row) // 同行
            {
                copyLines.push_back(m_lines[L.row].substr(L.col, R.col - L.col));
            }
            else // 跨行
            {
                copyLines.push_back(m_lines[L.row].substr(L.col));
                for (int i = L.row + 1; i < R.row; i++)
                {
                    copyLines.push_back(m_lines[i]);
                }
                copyLines.push_back(m_lines[R.row].substr(0, R.col));
            }
        }
    }

    std::string clipBoardText;
    for (const auto& str : copyLines) clipBoardText.append(str + '\n');
    ImGui::SetClipboardText(clipBoardText.c_str());
}

void NXTextEditor::Paste()
{
    std::string clipText = ImGui::GetClipboardText();
    std::vector<std::string> lines;
    size_t pos = 0;
    std::string token;
    while ((pos = clipText.find("\n")) != std::string::npos)
    {
        token = clipText.substr(0, pos);
        lines.push_back(token);
        clipText.erase(0, pos + 1);
    }
    lines.push_back(clipText);
    Enter({ lines });
}

void NXTextEditor::SelectAll()
{
    m_selections.assign(1, { {0, 0}, {(int)m_lines.size() - 1, (int)m_lines.back().length()} });
}

void NXTextEditor::HighLightSyntax(TextString& strLine)
{
    // 生成 某行代码文本的高亮
    static std::vector<std::vector<std::string>> const hlsl_tokens = {
        // comment
        {"//"},
        // values
        { "void", "true", "false", "bool", "int", "uint", "dword", "half", "float", "double", "min16float", "min10float", "min16int", "min12int", "min16uint", "bool1", "bool2", "bool3", "bool4", "int1", "int2", "int3", "int4", "uint1", "uint2", "uint3", "uint4", "dword1", "dword2", "dword3", "dword4", "half1", "half2", "half3", "half4", "float1", "float2", "float3", "float4", "double1", "double2", "double3", "double4", "min16float1", "min16float2", "min16float3", "min16float4", "min10float1", "min10float2", "min10float3", "min10float4", "min16int1", "min16int2", "min16int3", "min16int4", "min12int1", "min12int2", "min12int3", "min12int4", "min16uint1", "min16uint2", "min16uint3", "min16uint4", "float1x1", "float1x2", "float1x3", "float1x4", "float2x1", "float2x2", "float2x3", "float2x4", "float3x1", "float3x2", "float3x3", "float3x4", "float4x1", "float4x2", "float4x3", "float4x4", "double1x1", "double1x2", "double1x3", "double1x4", "double2x1", "double2x2", "double2x3", "double2x4", "double3x1", "double3x2", "double3x3", "double3x4", "double4x1", "double4x2", "double4x3", "double4x4", "vector", "matrix", "extern", "nointerpolation", "precise", "shared", "groupshared", "static", "uniform", "volatile", "const", "row_major", "column_major", "packoffset", "register" },
        // types
        { "string", "Buffer", "texture", "sampler", "sampler1D", "sampler2D", "sampler3D", "samplerCUBE", "sampler_state", "SamplerState", "SamplerComparisonState", "AppendStructuredBuffer", "Buffer", "ByteAddressBuffer", "ConsumeStructuredBuffer", "InputPatch", "OutputPatch", "RWBuffer", "RWByteAddressBuffer", "RWStructuredBuffer", "RWTexture1D", "RWTexture1DArray", "RWTexture2D", "RWTexture2DArray", "RWTexture3D", "StructuredBuffer", "Texture1D", "Texture1DArray", "Texture2D", "Texture2DArray", "Texture3D", "Texture2DMS", "Texture2DMSArray", "TextureCube", "TextureCubeArray" },
        // conditional branches
        { "if", "else", "for", "while", "do", "switch", "case", "default", "break", "continue", "discard", "return" },
        // methods
        { "abort", "abs", "acos", "all", "AllMemoryBarrier", "AllMemoryBarrierWithGroupSync", "any", "asdouble", "asfloat", "asin", "asint", "asuint", "atan", "atan2", "ceil", "CheckAccessFullyMapped", "clamp", "clip", "cos", "cosh", "countbits", "cross", "D3DCOLORtoUBYTE4", "ddx", "ddx_coarse", "ddx_fine", "ddy", "ddy_coarse", "ddy_fine", "degrees", "determinant", "DeviceMemoryBarrier", "DeviceMemoryBarrierWithGroupSync", "distance", "dot", "dst", "errorf", "EvaluateAttributeCentroid", "EvaluateAttributeAtSample", "EvaluateAttributeSnapped", "exp", "exp2", "f16tof32", "f32tof16", "faceforward", "firstbithigh", "firstbitlow", "floor", "fma", "fmod", "frac", "frexp", "fwidth", "GetRenderTargetSampleCount", "GetRenderTargetSamplePosition", "GroupMemoryBarrier", "GroupMemoryBarrierWithGroupSync", "InterlockedAdd", "InterlockedAnd", "InterlockedCompareExchange", "InterlockedCompareStore", "InterlockedExchange", "InterlockedMax", "InterlockedMin", "InterlockedOr", "InterlockedXor", "isfinite", "isinf", "isnan", "ldexp", "length", "lerp", "lit", "log", "log10", "log2", "mad", "max", "min", "modf", "msad4", "mul", "noise", "normalize", "pow", "printf", "Process2DQuadTessFactorsAvg", "Process2DQuadTessFactorsMax", "Process2DQuadTessFactorsMin", "ProcessIsolineTessFactors", "ProcessQuadTessFactorsAvg", "ProcessQuadTessFactorsMax", "ProcessQuadTessFactorsMin", "ProcessTriTessFactorsAvg", "ProcessTriTessFactorsMax", "ProcessTriTessFactorsMin", "radians", "rcp", "reflect", "refract", "reversebits", "round", "rsqrt", "saturate", "sign", "sin", "sincos", "sinh", "smoothstep", "sqrt", "step", "tan", "tanh", "tex1D", "tex1Dgrad", "tex1Dlod", "tex1Dproj", "tex2D", "tex2Dgrad", "tex2Dlod", "tex2Dproj", "tex3D", "tex3Dgrad", "tex3Dlod", "tex3Dproj", "texCUBE", "texCUBEgrad", "texCUBElod", "texCUBEproj", "transpose", "trunc" },
    };

    static std::vector<ImU32> hlsl_token_color = {
        0xffff9f4f, // values
        0xff00ffff, // types
        0xffff6fff, // conditional branches
        0xffffff4f, // methods
        0xff00ff00, // comments
    };

    std::vector<TextKeyword> strWords = ExtractKeywords(strLine);
    for(auto& strWord : strWords)
    {
        for (int i = 0; i < hlsl_tokens.size(); i++)
        {
            if (strWord.tokenColorIndex != -1) break;
            for (const auto& token : hlsl_tokens[i])
            {
                if (strWord.string == token)
                {
                    strWord.tokenColorIndex = i;
                    break;
                }
            }
        }
    }

    strLine.formatArray.clear();
    int idx = 0;
    for (const auto& strWord : strWords)
    {
        if (strWord.tokenColorIndex == -1) continue;

        int tokenLength = (int)strWord.string.length();
        if (strWord.startIndex > idx)
            strLine.formatArray.push_back(TextFormat(0xffffffff, strWord.startIndex - idx));

        strLine.formatArray.push_back(TextFormat(hlsl_token_color[strWord.tokenColorIndex], tokenLength));
        idx = strWord.startIndex + tokenLength;
    }
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
    ImVec2 windowPos = ImGui::GetWindowPos();
    ImVec2 contextArea = ImGui::GetContentRegionAvail();
    float scrollY = ImGui::GetScrollY();
    int startRow = (int)(scrollY / m_charHeight);
    int endRow = (int)((scrollY + contextArea.y) / m_charHeight);

    m_maxLineCharCount = 0;
    for (int i = 0; i < m_lines.size(); i++)
    {
        const auto& strLine = m_lines[i];
        m_maxLineCharCount = std::max(m_maxLineCharCount, strLine.length()); // 记录每行最长的字符数，用于填充空格

        if (i < startRow || i > endRow)
        {
            if (i == startRow - 1 || i == endRow + 1)
            {
                // 在视野外的上一行/下一行，使用最长行字符数量的空格填充，以保持固定长度（避免scrollX失效）
                ImGui::TextUnformatted(std::string(m_maxLineCharCount, ' ').c_str());
            }
            else
            {
                // 除上述行以外，跳过其他视野外的行的文本绘制，优化性能。
                ImGui::NewLine();
            }
            continue;
        }

        if (strLine.formatArray.empty())
            ImGui::TextUnformatted(strLine.c_str());
        else
        {
            int index = 0;
            for (const auto& strFormat : strLine.formatArray)
            {
                std::string strPart = strLine.substr(index, strFormat.length);
                index += strFormat.length;

                ImGui::PushStyleColor(ImGuiCol_Text, ImGui::ColorConvertU32ToFloat4(strFormat.color));
                ImGui::TextUnformatted(strPart.c_str());
                ImGui::PopStyleColor();

                if (index < strLine.length())
                    ImGui::SameLine();
                else
                    break;
            }

            if (index < strLine.length())
            {
                std::string strPart = strLine.substr(index);

                ImGui::PushStyleColor(ImGuiCol_Text, ImGui::ColorConvertU32ToFloat4(0xffffffff));
                ImGui::TextUnformatted(strPart.c_str());
                ImGui::PopStyleColor();
            }
        }
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

void NXTextEditor::CalcLineNumberRectWidth()
{
    int nLineNumberDigit = 0;
    for (int k = 1; k <= m_maxLineNumber; k *= 10) nLineNumberDigit++;

    // 记录行号文本能达到的最大宽度
    m_lineNumberWidth = m_charWidth * nLineNumberDigit;
    m_lineNumberWidthWithPaddingX = m_lineNumberWidth + m_lineNumberPaddingX * 2.0f;

    // 行号矩形 - 文本 之间留一个4px的空
    float paddingX = 4.0f;
    m_lineTextStartX = m_lineNumberWidthWithPaddingX + paddingX;
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
    bool overlayed = false; // 当 leftSignCount >= 2 时，说明有重叠产生，此时令 overlayed = true

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

int NXTextEditor::CalcSelectionLength(const SelectionInfo& selection)
{
    const auto& L = selection.L;
    const auto& R = selection.R;

    if (L.row == R.row)
    {
        auto& line = m_lines[L.row];
        int actualLcol = std::min(L.col, (int)line.size());
        int actualRcol = std::min(R.col, (int)line.size());
        return actualRcol - actualLcol;
    }
    else
    {
        auto& lineL = m_lines[L.row];
        auto& lineR = m_lines[R.row];
        int actualLcol = std::min(L.col, (int)lineL.size());
        int actualRcol = std::min(R.col, (int)lineR.size());
        int length = (int)lineL.size() - actualLcol + actualRcol;
        for (int i = L.row + 1; i < R.row - 1; i++)
            length += (int)m_lines[i].size();
        return length;
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
        bool bDeletePressed = ImGui::IsKeyPressed(ImGuiKey_Delete);
        bool bBackspacePressed = ImGui::IsKeyPressed(ImGuiKey_Backspace);
        bool bInsertPressed = ImGui::IsKeyDown(ImGuiKey_Insert);
        bool bEnterPressed = ImGui::IsKeyPressed(ImGuiKey_Enter);
        bool bEscPressed = ImGui::IsKeyPressed(ImGuiKey_Escape);

        bool bSelectAllCommand = bCtrl && ImGui::IsKeyPressed(ImGuiKey_A);
        bool bCopyCommand = bCtrl && ImGui::IsKeyPressed(ImGuiKey_C);
        bool bPasteCommand = bCtrl && ImGui::IsKeyPressed(ImGuiKey_V);

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
            MoveLeft(bShift, bCtrl, bKeyHomePressed, 1);
            m_bResetFlickerDt = true;
        }

        else if (!bAlt && (bKeyRightPressed || bKeyEndPressed))
        {
            MoveRight(bShift, bCtrl, bKeyEndPressed, 1);
            m_bResetFlickerDt = true;
        }

        else if (bDeletePressed || bBackspacePressed)
        {
            Backspace(bDeletePressed, bCtrl);
            m_bResetFlickerDt = true;
        }

        else if (bEnterPressed)
        {
            Enter({ {""}, {""} });
            m_bResetFlickerDt = true;
        }

        else if (bEscPressed)
        {
            Escape();
            m_bResetFlickerDt = true;
        }

        else if (bCopyCommand)
        {
            Copy();
            m_bResetFlickerDt = true;
        }

        else if (bPasteCommand)
        {
            Paste();
            m_bResetFlickerDt = true;
        }

        else if (bSelectAllCommand)
        {
            SelectAll();
            m_bResetFlickerDt = true;
        }

        if (!io.InputQueueCharacters.empty() && !bCtrl && !bAlt)
        {
            for (const auto& wc : io.InputQueueCharacters)
            {
                auto c = static_cast<char>(wc);
                if (wc >= 32 && wc < 127)
                {
                    Enter({{{c}}});
                    //Enter({ {"If it looks like food,", "it is not good food", "--senpai810"}, {"114 514", "1919810", "feichangdexinxian", "SOGOKUOISHII", "Desu!"} });
                    m_bResetFlickerDt = true;
                }
                else if (wc == '\t') // tab
                {
                    int tabSize = 4;
                    for (int i = 0; i < tabSize; ++i)
                    {
                        Enter({{" "}});
                        m_bResetFlickerDt = true;
                    }
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
        }
    }

    SelectionsOverlayCheckForKeyEvent(true);
    ScrollCheckForKeyEvent();
}

void NXTextEditor::MoveDown(bool bShift, bool bPageDown, bool bCtrlEnd)
{
    for (auto& sel : m_selections)
    {
        auto& pos = sel.flickerAtFront ? sel.L : sel.R;
        bCtrlEnd ? pos.row = (int)m_lines.size() : bPageDown ? pos.row += 20 : pos.row++;
        if (pos.row >= m_lines.size())
        {
            pos.row = (int)m_lines.size() - 1;
            pos.col = (int)m_lines[pos.row].size();
        }

        if (!bShift)
            sel.flickerAtFront ? sel.R = pos : sel.L = pos;
        else
        {
            if (sel.L > sel.R)
            {
                std::swap(sel.L, sel.R);
                sel.flickerAtFront = false;
            }
        }
    }

    SelectionsOverlayCheckForKeyEvent(false);
    ScrollCheckForKeyEvent();
}

void NXTextEditor::MoveLeft(bool bShift, bool bCtrl, bool bHome, int size)
{
    for (auto& sel : m_selections) MoveLeft(sel, bShift, bCtrl, bHome, size);
    SelectionsOverlayCheckForKeyEvent(true);
    ScrollCheckForKeyEvent();
}

void NXTextEditor::MoveRight(bool bShift, bool bCtrl, bool bEnd, int size)
{
    for (auto& sel : m_selections) MoveRight(sel, bShift, bCtrl, bEnd, size);
    SelectionsOverlayCheckForKeyEvent(false);
    ScrollCheckForKeyEvent();
}

void NXTextEditor::MoveLeft(SelectionInfo& sel, bool bShift, bool bCtrl, bool bHome, int size)
{
    auto& pos = sel.flickerAtFront ? sel.L : sel.R;
    pos.col = std::min(pos.col, (int)m_lines[pos.row].size());
    if (bHome) pos.col = 0;
    else
    {
        while (size) // 持续左移，直到 size 耗尽
        {
            if (pos.col > 0)
            {
                int newCol = std::max(pos.col - size, 0);
                size -= pos.col - newCol;
                pos.col = newCol;

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
                size--;
            }
            else break; // 如果已到达全文开头，则左移终止
        }
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
    }
}

void NXTextEditor::MoveRight(SelectionInfo& sel, bool bShift, bool bCtrl, bool bEnd, int size)
{
    auto& pos = sel.flickerAtFront ? sel.L : sel.R;
    pos.col = std::min(pos.col, (int)m_lines[pos.row].size());
    if (bEnd) pos.col = (int)m_lines[pos.row].size();
    else
    {
        while (size) // 持续右移，直到 size 耗尽
        {
            if (pos.col < m_lines[pos.row].size())
            {
                int newCol = std::min(pos.col + size, (int)m_lines[pos.row].size());
                size -= newCol - pos.col;
                pos.col = newCol;

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
                size--;
            }
            else break; // 如果已到达全文末尾，则右移终止
        }
    }

    if (!bShift)
        sel.flickerAtFront ? sel.R = pos : sel.L = pos;
    else
    {
        if (sel.L > sel.R)
        {
            std::swap(sel.L, sel.R);
            sel.flickerAtFront = false;
        }
    }
}

bool NXTextEditor::IsVariableChar(const char& ch)
{
    // ctrl+left/right 可以跳过的字符：
    return std::isalnum(ch) || ch == '_';
}

std::vector<NXTextEditor::TextKeyword> NXTextEditor::ExtractKeywords(const TextString& text)
{
    std::vector<NXTextEditor::TextKeyword> words;
    std::string word;
    int i;
    for(i = 0; i < text.length(); i++)
    {
        const char& c = text[i];
        // 对于字母或数字的字符，将其添加到当前单词
        if (std::isalnum(c) || c == '_') word += c;
        else if (!word.empty())
        {
            // 对于非字母或数字的字符，如果当前单词不为空，则添加到words列表中，并清空当前单词
            words.push_back({ word, i - (int)word.length() });;
            word.clear();
        }
    }

    // 如果最后一个单词没有被添加到words列表中，则需要在循环结束后再添加一次
    if (!word.empty()) words.push_back({ word, i - (int)word.length() });
    return words;
}
