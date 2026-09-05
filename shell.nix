{ pkgs ? import <nixpkgs> {} }:

pkgs.mkShell {
  name = "tti-launcher-build";

  buildInputs = with pkgs; [
    cmake
    ninja
    gcc
    qt6.qtbase
    qt6.qttools
    bzip2
    pkg-config
  ];

  shellHook = ''
    export CMAKE_PREFIX_PATH="${pkgs.qt6.qtbase}:$CMAKE_PREFIX_PATH"
  '';
}