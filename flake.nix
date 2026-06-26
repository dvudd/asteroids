{
  description = "Asteroids SFML Development environment";

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

        libxrandr
        libxcursor
        libxi
        libx11
        libxext
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
        libx11
        libxrandr
        libxcursor
        libxi
        libxext
      ]);
      shellHook = ''
        echo "Good luck, Have fun!"
        export SHELL=$(which zsh)
      '';
    };
  };
}
