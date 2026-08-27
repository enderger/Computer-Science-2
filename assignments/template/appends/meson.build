test('week@@WEEK@@-problem@@PROBLEM@@',
  executable('week@@WEEK@@-problem@@PROBLEM@@-tests', 'week@@WEEK@@-problem@@PROBLEM@@.cpp',
    cpp_args: ['-DDOCTEST_CONFIG_IMPLEMENT_WITH_MAIN'],
    dependencies: doctest_dep,
    include_directories: incdir))

executable('week@@WEEK@@-problem@@PROBLEM@@', 'week@@WEEK@@-problem@@PROBLEM@@.cpp', 'week@@WEEK@@-problem@@PROBLEM@@-runner.cpp',
    cpp_args: ['-DDOCTEST_CONFIG_DISABLE'],
    dependencies: doctest_dep,
    include_directories: incdir,
    install: true)
