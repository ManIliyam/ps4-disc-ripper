# GUI distribution notices

The GUI uses Scene2D graphics.cpp, graphics.h and log.h copied from the installed
OpenOrbis v0.5.4 samples. The SDK's root LICENSE is GPL version 3. The combined
GUI is distributed under GPL-3.0 terms; the original project files remain available
under their MIT license. The renderer's license is included in the package under
assets/credits and in the corresponding source distribution under licenses.
The source distribution includes the exact sample-renderer files used in the build.

The Gontserrat font is supplied by the SDK font sample. Its OFL and copyright
notices are included next to the font inside the package and in the source archive.

The OpenOrbis SDK and its individual tool/runtime dependencies retain their own
licenses. The toolchain itself is not redistributed in the source ZIP. The build
instructions identify the public toolchain release and the package-tool source used.
No official Sony SDK is required by our source build.

PicoSHA2 is vendored in src/extraction/picosha2.h at upstream revision
161cb3fc4170fa7a3eca9e582cebd27cc4d1fe29, under its MIT license. Both the header's
notice and PicoSHA2-LICENSE are retained; the package also includes its notice.

The font renderer has a project patch for unsigned glyph coverage, bitmap pitch
and blending over the current framebuffer. The exact patched files are included
in the corresponding-source archive. The installed SDK remains unmodified.
