project('uml-server', 'cpp', version : '0.1.2', default_options: ['cpp_std=c++23'])
yaml_cpp = dependency('yaml-cpp')
include_dir = include_directories('include')
egm = dependency('egm')
uml_cpp = dependency('uml-cpp')
uml_server_lib = library('uml-server-protocol', 
    'src/uml-server/umlServer.cpp', 'src/uml-server/serverPersistencePolicy.cpp', 'src/uml-server/umlClient.cpp', 'src/uml-server/metaManager.cpp', 'src/uml-server/generativeSerializationPolicy.cpp',
    include_directories : include_dir, 
    dependencies: [egm, uml_cpp, yaml_cpp]
)
executable('uml-server', 
    'src/uml-server/main.cpp', 
    link_with : uml_server_lib,
    include_directories : include_dir,
    dependencies: [egm, uml_cpp, yaml_cpp])
if (get_option('serverTests'))
    gtest = dependency('gtest', main : true, required : false)
    uml_server_tests = executable('uml-server-tests', 
        'src/test/umlServerTest.cpp', 'src/test/metaManagerTest.cpp', 'src/test/generativeManagerTest.cpp',
        link_with : uml_server_lib, 
        include_directories : include_dir, 
        dependencies : [egm, gtest, uml_cpp, yaml_cpp],
        cpp_args: ['-fsanitize=address', '-fno-omit-frame-pointer'],
        link_args: '-fsanitize=address'
    )
    test('uml-server-tests', uml_server_tests)
endif
