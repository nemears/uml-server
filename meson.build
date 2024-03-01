project('uml-server', 'cpp', version : '0.0.2')
yaml_cpp = dependency('yaml-cpp')
include_dir = include_directories('include')

uml_cpp = dependency('uml-cpp')
uml_server_lib = library('uml-server-protocol', 
    'src/uml-server/umlServer.cpp', 'src/uml-server/serverPersistencePolicy.cpp', 'src/uml-server/umlClient.cpp', 
    include_directories : include_dir, 
    dependencies: [uml_cpp, yaml_cpp]
)
executable('uml-server', 
    'src/uml-server/main.cpp', 
    link_with : uml_server_lib, 
    include_directories : include_dir,
    dependencies: uml_cpp)
if (get_option('serverTests'))
    gtest = dependency('gtest', main : true, required : false)
    uml_server_tests = executable('uml-server-tests', 
        'src/test/umlServerTest.cpp', 
        link_with : uml_server_lib, 
        include_directories : include_dir, 
        dependencies : [gtest, uml_cpp]
    )
    test('uml-server-tests', uml_server_tests)
endif
