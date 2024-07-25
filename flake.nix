{
    description = "A server to host a single instance of a uml model";

    inputs = {
        flake-utils.url = "github:numtide/flake-utils";
        uml-cpp.url = "github:nemears/uml-cpp";
        nixpkgs.follows = "uml-cpp/nixpkgs";
    };

    outputs = { self, nixpkgs, flake-utils, uml-cpp }:
    flake-utils.lib.eachDefaultSystem
        (system:
            let 
              pkgs = nixpkgs.legacyPackages.${system};
              mkUmlServer = { src, umlcpp } : pkgs.stdenv.mkDerivation {
                    name = "uml-server";
                    inherit src umlcpp;
                    buildInputs = with pkgs; [meson pkg-config coreutils yaml-cpp umlcpp ninja];
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
 
            in
            {
                devShells.default = pkgs.mkShell {
                    packages = with pkgs; [ 
                    # build dependencies
                    meson
                    pkg-config
                    ninja

                    # dev dependencies
                    yaml-cpp
                    gtest
                    uml-cpp.outputs.packages.${system}.default

                    # debug dependencies
                    gdb
                    valgrind
                    ];
                };

                packages.uml-server = mkUmlServer {
                  src = ./.;
                  umlcpp = uml-cpp.outputs.packages.${system}.default;
                };
                packages.uml-server_0_1_1 = mkUmlServer {
                  src = builtins.fetchGit {
                    url = "ssh://git@github.com/nemears/uml-server.git";
                    rev = "3855f949036232b88c54b36809ae9205cac9532f";
                  };
                  umlcpp = uml-cpp.outputs.packages.${system}.uml-cpp_0_3_6;
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
