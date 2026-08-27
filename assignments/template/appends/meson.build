test('week@@WEEK@@-day@@DAY@@',
  executable('week@@WEEK@@-day@@DAY@@-tests', 'week@@WEEK@@-day@@DAY@@.cpp',
    cpp_args: ['-DDOCTEST_CONFIG_IMPLEMENT_WITH_MAIN'],
    dependencies: doctest_dep,
    include_directories: incdir))

executable('week@@WEEK@@-day@@DAY@@', 'week@@WEEK@@-day@@DAY@@.cpp', 'week@@WEEK@@-day@@DAY@@-runner.cpp',
    cpp_args: ['-DDOCTEST_CONFIG_DISABLE'],
    dependencies: doctest_dep,
    include_directories: incdir)
