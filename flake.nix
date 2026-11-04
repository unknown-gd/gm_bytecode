{
  description = "gm_bytecode — Garry's Mod bytecode loader";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
  };

  outputs =
    { self, nixpkgs }:
    let
      # GMod Linux modules are x86/x86_64 only.
      systems = [ "x86_64-linux" ];
      forAllSystems = nixpkgs.lib.genAttrs systems;

      mkGmBytecode =
        {
          stdenv,
          cmake,
          lib,
          pnameSuffix ? "",
          metaPlatforms ? lib.platforms.linux,
        }:
        stdenv.mkDerivation {
          pname = "gm_bytecode${pnameSuffix}";
          version = "0.0.1";

          src = lib.cleanSource ./.;

          nativeBuildInputs = [ cmake ];

          cmakeBuildType = "Release";

          cmakeFlags = [ "-DGM_BYTECODE_TARGET=auto" ];

          installPhase = ''
            runHook preInstall
            mkdir -p $out/lib
            cp -v gmsv_bytecode_*.dll gmcl_bytecode_*.dll $out/lib/
            runHook postInstall
          '';

          meta = with lib; {
            description = "Bytecode loader binary module for Garry's Mod (server + client)";
            license = licenses.mit;
            platforms = metaPlatforms;
          };
        };
    in
    {
      packages = forAllSystems (
        system:
        let
          pkgs = nixpkgs.legacyPackages.${system};

          linux64 = pkgs.callPackage mkGmBytecode {
            pnameSuffix = "-linux64";
            metaPlatforms = [ "x86_64-linux" ];
          };

          linux32 = pkgs.pkgsi686Linux.callPackage mkGmBytecode {
            pnameSuffix = "-linux32";
            metaPlatforms = [ "i686-linux" ];
          };

          all = pkgs.symlinkJoin {
            name = "gm_bytecode-all";
            paths = [
              linux64
              linux32
            ];
            meta = {
              description = "gm_bytecode GMod modules (gmsv_/gmcl_, linux32 + linux64)";
            };
          };
        in
        {
          inherit linux64 linux32 all;
          default = all;
        }
      );

      devShells = forAllSystems (
        system:
        let
          pkgs = nixpkgs.legacyPackages.${system};
        in
        {
          default = pkgs.mkShell {
            packages = [
              pkgs.cmake
              pkgs.ninja
              pkgs.gcc
              pkgs.pkgsi686Linux.gcc
            ];
          };
        }
      );
    };
}
