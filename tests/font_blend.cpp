#include "../src/ui/FontBlend.h"
#include <cassert>
int main() {
    for(unsigned background=0;background<256;++background)
        for(unsigned foreground=0;foreground<256;++foreground) {
            assert(fontBlend(background,foreground,255)==foreground);
            assert(fontBlend(background,foreground,0)==background);
            assert(fontBlend(background,foreground,128)==(background*127+foreground*128+127)/255);
        }
}
