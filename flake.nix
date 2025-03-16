{
    description = "A server to host a single instance of a uml model";

    inputs = {
        flake-utils.url = "github:numtide/flake-utils";
        uml-cpp.url = "github:nemears/uml-cpp";
        nixpkgs.follows = "uml-cpp/nixpkgs";
        # egm.follows = "uml-cpp/egm";
        egm.url = "path:/home/emory/Projects/egm";
    };

    outputs = { self, nixpkgs, flake-utils, uml-cpp, egm }:
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
                    # uml-cpp.outputs.packages.${system}.uml-cpp_0_4_2
                    uml-cpp.outputs.packages.${system}.default
                    egm.outputs.packages.${system}.default

                    # debug dependencies
                    gdb
                    valgrind
                    ];
                };

                packages.uml-server = mkUmlServer {
                  src = ./.;
                  umlcpp = uml-cpp.outputs.packages.${system}.uml-cpp_0_4_2;
                };
                packages.uml-server_0_1_2 = pkgs.stdenvNoCC.mkDerivation {
                  src = pkgs.fetchurl {
                    url = "https://api.github.com/repos/nemears/uml-server/releases/assets/197656302";
                    hash = "sha256-g4wDhRI7LR+fbgcpEyWWwhlj6NMESvXwIYpyJ0Ahzb4=";
                    curlOptsList = [
                      "-L"
                      "--verbose"
                      "-HAccept: application/octet-stream"
                      "-HAuthorization: Bearer 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"
                      "-HX-GitHub-Api-Version: 2022-11-28"
                    ];
                    # postFetch = ''
                    #   tar xvzf $downloadedFile 
                    #   ls -la
                    #   rm $out
                    #   cp -r uml-server-v0.1.2 $out
                    # '';
                  };
                  name = "uml-server";
                  unpackPhase = ''
                    tar xvzf $src
                    mkdir $out
                    cp -rp uml-server-release/. $out
                  '';
		  umlcpp = uml-cpp.outputs.packages.${system}.uml-cpp_0_4_2;
		  yamlcpp = pkgs.yaml-cpp;
	   	  buildPhase = ''
		    ln -sf $umlcpp/lib/libuml.so $out/bin/libuml.so
		    ln -sf $yamlcpp/lib/libyaml-cpp.so $out/bin/libyaml-cpp.so.0.8
		  '';
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
