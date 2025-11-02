internal_test_compiler_flags = compiler_flags + ['-DTVG_STATIC']

internal_test_dep = []
if host_machine.system() == 'darwin'
    internal_test_dep += declare_dependency(link_args: ['-framework', 'Cocoa', '-framework', 'IOKit'])
endif

internal_test_file = [
    'testInternalMain.cpp',
    'testGeometry.cpp'
]

internal_tests = executable('tvgInternalTests',
    internal_test_file,
    include_directories : headers,
    dependencies : [thorvg_lib_dep, internal_test_dep],
    cpp_args : internal_test_compiler_flags)

test('Internal Unit Tests', internal_tests, args : ['--success'])

