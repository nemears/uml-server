project('uml-server', 'cpp', version : '0.0.1')
umlCpp = dependency('uml-cpp')
yamlCpp = dependency('yaml-cpp')
gtest = dependency('gtest')
includeDir = include_directories('include')
umlServerLib = static_library('uml-server-protocol', 
    'src/uml-server/umlServer.cpp', 'src/uml-server/serverPersistencePolicy.cpp', 'src/uml-server/umlClient.cpp', 
    include_directories : includeDir, 
    dependencies: [umlCpp, yamlCpp])
executable('uml-server', 'src/uml-server/main.cpp', link_with : umlServerLib, include_directories : includeDir)
