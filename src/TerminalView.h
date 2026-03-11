#pragma once
#include "stdafx.h"

enum class DisplayMode { ASCII, HEX, MIXED };

// A line entry in the terminal log
struct TerminalLine {
    std::vector<BYTE> raw;
    bool              isRx;       // true=received, false=sent
    SYSTEMTIME        timestamp;
};

// Custom owner-draw listbox for terminal output
class CTerminalView : public CListBox
{
public:
    CTerminalView();
    virtual ~CTerminalView();

    void AppendData(const std::vector<BYTE>& data, bool isRx);
    void Clear();
    void SetDisplayMode(DisplayMode mode);
    void SetShowTimestamp(bool show)   { m_showTimestamp = show; Invalidate(); }
    void SetShowLineNumbers(bool show) { m_showLineNumbers = show; Invalidate(); }
    void SetMaxLines(int n)            { m_maxLines = n; }
    void SetFont(const LOGFONT& lf);
    void CopySelectedToClipboard();
    void SaveToFile(const CString& path);
    DisplayMode GetDisplayMode() const { return m_mode; }

protected:
    virtual void DrawItem(LPDRAWITEMSTRUCT lpDIS) override;
    virtual void MeasureItem(LPMEASUREITEMSTRUCT lpMIS) override;
    virtual void PreSubclassWindow() override;

    DECLARE_MESSAGE_MAP()
    afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
    afx_msg void OnCopy();
    afx_msg void OnSelectAll();
    afx_msg void OnClear();

private:
    std::wstring FormatLine(const TerminalLine& line) const;
    std::wstring FormatHex(const std::vector<BYTE>& data) const;
    std::wstring FormatMixed(const std::vector<BYTE>& data) const;
    std::wstring FormatTimestamp(const SYSTEMTIME& st) const;

    std::vector<TerminalLine> m_lines;
    DisplayMode  m_mode          = DisplayMode::ASCII;
    bool         m_showTimestamp = true;
    bool         m_showLineNumbers = false;
    int          m_maxLines      = 5000;
    int          m_lineHeight    = 16;
    CFont        m_font;

    // Colors
    COLORREF m_colRxText   = RGB(220, 255, 220);
    COLORREF m_colTxText   = RGB(220, 220, 255);
    COLORREF m_colBg       = RGB(18, 18, 18);
    COLORREF m_colSel      = RGB(50, 80, 120);
    COLORREF m_colTimestamp= RGB(120, 120, 120);
    COLORREF m_colHexAddr  = RGB(180, 140, 60);
};
