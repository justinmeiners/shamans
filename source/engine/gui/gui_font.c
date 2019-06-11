
#include "gui_font.h"

void GuiFont_Init(GuiFont* font, int charsPerRow, int charCount)
{
    assert(font);
    
    /* figure out UV coordinates for each ASCII character */
    const float fCharsPerRow = (float)charsPerRow;
    const float squareSize = 1.0f / fCharsPerRow;
    
    for (int i = 0; i < charCount; ++i)
    {
        float cx = (i % charsPerRow) / fCharsPerRow;
        float cy =  (i / charsPerRow) / fCharsPerRow;
        
        int index = i * 2;
        
        font->uvs[index].x = cx;
        font->uvs[index].y = cy + squareSize;
        
        font->uvs[index + 1].x = cx + squareSize;
        font->uvs[index + 1].y = cy;
        
        font->charOffsets[i] = Vec2_Zero;
    }
}

void GuiFont_Shutdown(GuiFont* font)
{

}
