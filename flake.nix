{
  description = "SFML C++ project dev shell";

  inputs.nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";

  outputs = { nixpkgs, ... }:
  let
    system = "x86_64-linux";
    pkgs = import nixpkgs { inherit system; };
  in {
    devShells.${system}.default = pkgs.mkShell {
      packages = with pkgs; [
        cmake
        ninja
        gcc
        git
        pkg-config
        clang-tools

        xorg.libXrandr
        xorg.libXcursor
        xorg.libXi
        xorg.libX11
        xorg.libXext
        udev
        freetype
        flac
        libvorbis
        libGL
        libglvnd
        mesa
        harfbuzz
        mbedtls
        libssh2
      ];

      LD_LIBRARY_PATH = pkgs.lib.makeLibraryPath (with pkgs; [
        libGL
        libglvnd
        mesa
        udev
        xorg.libX11
        xorg.libXrandr
        xorg.libXcursor
        xorg.libXi
        xorg.libXext
      ]);
    };
  };
}
