# Build PS4 Disc Ripper v1.15

Use Linux or WSL2 with Python 3, GNU make, clang++ 18, lld and an installed
OpenOrbis binary toolchain containing headers, libraries, Linux tools and samples.
No official Sony SDK is required. Obtain it through [OpenOrbis releases](https://github.com/OpenOrbis/OpenOrbis-PS4-Toolchain/releases).

## Recorded inputs

- Ubuntu 24.04 / WSL2; clang 18.1.3; .NET SDK 8.0.132.
- Archive `toolchain-llvm-18.tar.gz`, SHA-256:
  `3c7cd5bb593ca74fa1c13fd59f3938dc0fc07985167f7275063019e63abe4526`.
- LibOrbisPkg commit `92a4e7f8e5914cb06b52ba3ae6b0675da438a051`.
- PicoSHA2 commit `161cb3fc4170fa7a3eca9e582cebd27cc4d1fe29`.

The archive may contain nested `OpenOrbis/PS4Toolchain` directories. Set the SDK
path to the folder containing `include`, `lib`, `link.x`, `bin` and `samples`.
A source-only SDK clone is insufficient.

## Commands

Run in Linux/Ubuntu, not PowerShell:

```bash
export OO_PS4_TOOLCHAIN=/absolute/path/to/OpenOrbis/PS4Toolchain
export DOTNET_SYSTEM_GLOBALIZATION_INVARIANT=1
git clone --recurse-submodules https://github.com/OpenOrbis/LibOrbisPkg.git
git -C LibOrbisPkg checkout 92a4e7f8e5914cb06b52ba3ae6b0675da438a051
dotnet build LibOrbisPkg/PkgTool.Core/PkgTool.Core.csproj -c Release

# From the PS4 Disc Ripper source root:
python3 tools/prepare_gui.py
make -C build/gui-v06 pkg.gp4
python3 tools/package_gui.py --pkg-tool /absolute/path/to/LibOrbisPkg/PkgTool.Core/bin/Release/net8.0/PkgTool.Core.dll
```

The project uses C++17. Staging copies the installed hello_world sample, adds
project sources, applies the font-renderer fix and copies icon/fonts/notices.
It never edits the SDK and refuses to overwrite an existing build folder. Use a
fresh checkout for a clean reconstruction. The corresponding-source ZIP includes
the exact staged renderer files and font.

The .NET 8 tool avoids ICU/OpenSSL failures in the older bundled package runtime.
Do not install obsolete libssl packages. Outputs in `dist/` include PKG, source
ZIP, hashes, metadata and validation logs; all package checks must report `[OK]`.
Package timestamps may prevent bit-identical rebuilds. Compilation/validation do
not establish compatibility on a console.

## Host checks

```bash
mkdir -p build
clang++ -std=c++17 -O1 -fsanitize=address,undefined tests/stream_copy.cpp -o build/stream-copy-test
./build/stream-copy-test
clang++ -std=c++17 -O1 -fsanitize=address,undefined tests/ftp_inspect.cpp -o build/ftp-inspect-test
./build/ftp-inspect-test
clang++ -std=c++17 tests/font_blend.cpp -o build/font-blend-test
./build/font-blend-test
clang++ -std=c++17 -O1 tests/gui_host.cpp -o build/gui-host-test
python3 tools/check_gui_host.py
```

These check portable failures, hashes, file-inspection decisions, font coverage
and UI traces. They do not emulate native PS4 optical I/O.

Runtime module sources are public in the OpenOrbis repository under
`src/modules/libc`, `src/modules/libSceFios2` and `src/modules/right`. The toolchain
remains a separately installed dependency; retain its licenses and notices.
