{
    description = "A server to host a single instance of a uml model";

    inputs = {
        nixpkgs.url = "github:NixOS/nixpkgs/release-23.05";
        flake-utils.url = "github:numtide/flake-utils";
        uml-cpp.url = "github:nemears/uml-cpp";
    };

    outputs = { self, nixpkgs, flake-utils, uml-cpp }:
    flake-utils.lib.eachDefaultSystem
        (system:
            let pkgs = nixpkgs.legacyPackages.${system}; in
            {
                devShells.default = pkgs.mkShell {
                    packages = with pkgs; [ 
                    # build dependencies
                    meson

                    # dev dependencies
                    yaml-cpp
                    uml-cpp.outputs.packages.${system}.default

                    # debug dependencies
                    gdb
                    valgrind
                    ];
                };

                packages.uml-server = pkgs.stdenv.mkDerivation {
                    name = "uml-server";
                    src = ./.;
                    buildInputs = with pkgs; [meson pkg-config coreutils yaml-cpp uml-cpp.outputs.packages.${system}.default ninja];
                    umlcpp = uml-cpp.outputs.packages.${system}.default;
                    preConfigure = ''
                    export PKG_CONFIG_PATH_FOR_TARGET=$umlcpp/lib:$PKG_CONFIG_PATH_FOR_TARGET
                    echo $PKG_CONFIG_PATH_FOR_TARGET
                    pkg-config --list-all
                    '';
                    postConfigure = ''
                    meson configure -DserverTests=false
                    '';
                    installPhase = 
                    ''
                    mkdir -p $out/lib $out/include $out/bin
                    cp uml-server $out/bin
                    cp libuml-server-protocol.so $out/lib
                    cp -r $src/include/uml-server $out/include
                    '';
                };

                packages.default = self.packages.${system}.uml-server;

                apps.uml-server = {
                    type = "app";
                    program = "${self.packages.${system}.uml-server}/bin/uml-server";
                };

                apps.default = self.apps.${system}.uml-server;
            }
        );
}