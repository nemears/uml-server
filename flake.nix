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
              mkUmlServer = { src, umlcpp ? uml-cpp.outputs.packages.${system}.default } : pkgs.stdenv.mkDerivation {
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
                };
                packages.uml-server_0_1_1 = mkUmlServer {
                  src = builtins.fetchGit {
                    url = "ssh://git@github.com/nemears/uml-server.git";
                    rev = "9116beadcb61eb6657a222d7b721d3aa690b04e3";
                  };
                  umlcpp = uml-cpp.outputs.packages.${system}.uml-cpp_0_3_5;
                };

                packages.default = self.packages.${system}.uml-server;

                apps.uml-server = {
                    type = "app";
                    program = "${self.packages.${system}.uml-server}/bin/uml-server";
                };
		apps.uml-server_0_1_1 = {
                  type = "app";
                  program = "${self.packages.${system}.uml-server_0_1_1}/bin/uml-server";
                };

                apps.default = self.apps.${system}.uml-server;
            }
        );
}
