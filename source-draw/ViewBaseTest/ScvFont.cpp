#include <ViewBaseTest/ScvFont.h>

#include <Viewer/WasmViewer.h>

namespace ViewBaseTest {

// =======================================================================
// function :ScvFont
// =======================================================================
ScvFont::ScvFont()
{
    AddOnOffNoKey(this, "-override") = &m_toOverride;
}

// =======================================================================
// function : ParseAllArguments
// =======================================================================
bool ScvFont::ParseAllArguments()
{
    if (!DrawCommand::ParseAllArguments()) {
        return false;
    }
    if (m_fontFile.empty()) {
        *myDI << "Syntax error: wrong number of arguments";
        return false;
    }
    return true;
}

// =======================================================================
// function : ParseArguments
// =======================================================================
bool ScvFont::ParseArgument(int& theArgIter)
{
    if (m_fontFile.empty()) {
        m_fontFile = Argument(theArgIter);
    } else if (m_fontName.empty()) {
        m_fontName = Argument(theArgIter);
    } else {
        return false;
    }
    return true;
}

// =======================================================================
// function : Perform
// =======================================================================
bool ScvFont::Perform()
{
    Viewer::WasmViewer::_cppRegisterFont(m_fontFile, m_fontName, m_toOverride);
    return true;
}

}

