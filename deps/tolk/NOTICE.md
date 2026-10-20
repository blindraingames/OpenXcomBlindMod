# Tolk runtime notice

The bundled Tolk runtime is built from the official Tolk source repository:

- Source: https://github.com/dkager/tolk
- Revision source used for this build: repository `master` archive retrieved on 2026-09-24; the corresponding source files are included in `deps/tolk/src`
- License: GNU Lesser General Public License version 3; see `LICENSE.txt`

The NVDA controller clients are the official binaries distributed with NVDA 2026.2:

- Source and binary release: https://download.nvaccess.org/releases/2026.2/
- Package: `nvda_2026.2_controllerClient.zip`
- License: GNU Lesser General Public License version 2.1; see `LICENSE-NVDA.txt`

The controller DLLs retain the legacy `nvdaControllerClient32.dll` and `nvdaControllerClient64.dll` filenames expected by Tolk.
