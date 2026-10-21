"""Patch the staged OpenOrbis renderer; never modify the installed SDK."""
def fix_font_renderer(source):
    start=source.rfind('\n',0,source.index('// Decode the 8-bit bitmap'))+1
    end=source.index('\n',source.index('this->DrawPixel(x, y, finalColor);',start))
    replacement='''                  // FreeType coverage must stay unsigned: signed char makes
                  // opaque glyph interiors black on x86-64.
                  const unsigned char* row = slot->bitmap.buffer + yPos * slot->bitmap.pitch;
                  const uint8_t coverage = row[xPos];
                  const int x = startX + xPos + xOffset + slot->bitmap_left;
                  const int y = startY + yPos + yOffset - slot->bitmap_top;
                  if (!coverage || x < 0 || y < 0 || x >= this->width || y >= this->height)
                      continue;
                  const uint32_t background = ((uint32_t*)this->frameBuffers[this->activeFrameBufferIdx])[y*this->width+x];
                  Color blended = {
                      fontBlend(static_cast<uint8_t>(background >> 16), fgColor.r, coverage),
                      fontBlend(static_cast<uint8_t>(background >> 8), fgColor.g, coverage),
                      fontBlend(static_cast<uint8_t>(background), fgColor.b, coverage)
                  };
                  this->DrawPixel(x, y, blended);'''
    return '#include "FontBlend.h"\n'+source[:start]+replacement+source[end:]
