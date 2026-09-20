{
  description = "Temporary dev environment for Exorcist IDE (uncommitted)";

  inputs.nixpkgs.url = "github:NixOS/nixpkgs/nixpkgs-unstable";

  outputs = { self, nixpkgs }:
    let
      system = "x86_64-linux";
      pkgs = import nixpkgs { inherit system; };
      qt = pkgs.qt6;
    in {
      devShells.${system}.default = pkgs.mkShell {
        packages = with pkgs; [
          cmake
          ninja
          gcc
          gnumake
          pkg-config
          git
          python3
          qt.qtbase
          qt.qtsvg
          qt.qtdeclarative
          qt.qttools
          qt.qtserialport
          # System JavaScriptCore provider for the headless JS plugin runtime
          # when Ultralight is disabled (pkg-config: javascriptcoregtk-4.1).
          webkitgtk_4_1
          libGL
          libglvnd
          xorg.libX11
          xorg.libxcb
          xorg.xcbutil
          xorg.xcbutilimage
          xorg.xcbutilkeysyms
          xorg.xcbutilrenderutil
          xorg.xcbutilwm
          fontconfig
          freetype
        ];

        shellHook = ''
          export QT_QPA_PLATFORM=offscreen
          export QT_PLUGIN_PATH="${qt.qtbase}/lib/qt-6/plugins"
          echo "Exorcist dev shell: $(cmake --version | head -1)"
        '';
      };
    };
}