{
  description = "Dev shell for vulkit";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-25.05";
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
          cmake
          clang-tools
          clang
          fmt
          hwloc
          linuxPackages.perf
          gnumake
          python313
          vulkan-loader
          vulkan-headers
          vulkan-tools
          vulkan-memory-allocator
          spirv-tools
          shaderc
        ];
        shellHook = ''
          export SHELL=${pkgs.zsh}/bin/zsh
        '';
      };
    };
}
