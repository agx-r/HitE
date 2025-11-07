{
  description = "HitE";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
    flake-utils.url = "github:numtide/flake-utils";
  };

  outputs = { self, nixpkgs, flake-utils }:
    flake-utils.lib.eachDefaultSystem (system:
      let
        pkgs = nixpkgs.legacyPackages.${system};
        
        buildInputs = with pkgs; [
          vulkan-loader
          vulkan-headers
          vulkan-validation-layers
          shaderc
          glfw
          glm
        ];
        
        nativeBuildInputs = with pkgs; [
          cmake
          ninja
          pkg-config
          clang
          llvm
          glslang
        ];

      in {
        packages.default = pkgs.stdenv.mkDerivation {
          pname = "hite";
          version = "0.1.0";
          src = ./.;

          inherit buildInputs nativeBuildInputs;

          cmakeFlags = [
            "-DCMAKE_BUILD_TYPE=Release"
            "-DCMAKE_EXPORT_COMPILE_COMMANDS=ON"
            "-DCMAKE_INSTALL_PREFIX=$out"
          ];
          
          installPhase = ''
            cmake --install . --prefix $out
          '';
        };

        devShells.default = pkgs.mkShell {
          inherit buildInputs;
          
          nativeBuildInputs = nativeBuildInputs ++ (with pkgs; [
            gdb
            valgrind
            renderdoc
          ]);

          shellHook = ''
            export VK_LAYER_PATH="${pkgs.vulkan-validation-layers}/share/vulkan/explicit_layer.d"
            export LD_LIBRARY_PATH="${pkgs.lib.makeLibraryPath buildInputs}:$LD_LIBRARY_PATH"
            echo "dev env loaded"
            echo "Vulkan version: $(vulkaninfo --summary 2>/dev/null | grep 'Vulkan Instance Version' || echo 'Not available')"
          '';
        };
      }
    );
}
