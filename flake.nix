{
  description = "Dev shell for vulkit";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
  };

  outputs =
    { self, nixpkgs }:
    let
      system = "x86_64-linux";
      pkgs = import nixpkgs { inherit system; };
    in
    {
      devShells.${system}.default = pkgs.mkShell {
        name = "vulkit";

        buildInputs = with pkgs; [
          clang
          clang-tools
          lld
          libcxx

          cmake
          fmt
          pkg-config
          hwloc
          perf
          gnumake
          python313

          vulkan-loader
          vulkan-headers
          vulkan-tools
          vulkan-memory-allocator
          vulkan-validation-layers

          spirv-tools
          shaderc
        ];
        shellHook = ''
          export SHELL=${pkgs.zsh}/bin/zsh
          export CC=clang
          export CXX=clang++
          export CLANGD_FLAGS="$CLANGD_FLAGS --query-driver=/nix/store/*-clang-wrapper-*/bin/clang++"
        '';
      };
    };
}
