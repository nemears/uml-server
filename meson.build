project('uml-server', 'cpp', version : '0.0.1')
umlCpp = dependency('uml-cpp')
yamlCpp = dependency('yaml-cpp')
gtest = dependency('gtest', main : true, required : false)
includeDir = include_directories('include')
umlServerLib = static_library('uml-server-protocol', 
    'src/uml-server/umlServer.cpp', 'src/uml-server/serverPersistencePolicy.cpp', 'src/uml-server/umlClient.cpp', 
    include_directories : includeDir, 
    dependencies: [umlCpp, yamlCpp]
)
executable('uml-server', 'src/uml-server/main.cpp', link_with : umlServerLib, include_directories : includeDir)
umlServerTests = executable('uml-server-tests', 
    'src/test/umlServerTest.cpp', 
    link_with : umlServerLib, 
    include_directories : includeDir, 
    dependencies : gtest
)
test('uml-server-tests', umlServerTests)
