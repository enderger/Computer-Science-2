# Copyright 2026 Danielle Hutzley
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#   http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
# Generated from template
#
project(
    'cs2-lib',
    'cpp',
    version: '0.1.0',
    default_options: ['cpp_std=c++23', 'warning_level=3', 'werror=true'],
    license: 'Apache-2.0',
)

cpptrace_dep = dependency('cpptrace')
doctest_dep = dependency('doctest')

inc_dir = include_directories('include')
src = ['hash.cpp', 'sexp.cpp', 'util.cpp']

if get_option('buildtype') != 'release'
    add_project_arguments('-DCS2LIB_DEBUG', language: 'cpp')
    add_project_dependencies(cpptrace_dep, language: 'cpp')
endif

cs2_lib = static_library(
    'cs2-lib',
    src,
    include_directories: inc_dir,
    install: true,
    cpp_args: ['-DDOCTEST_CONFIG_DISABLE'],
    dependencies: doctest_dep,
)
install_headers(
    'include/cs2lib/hash.hpp',
    'include/cs2lib/sexp.hpp',
    'include/cs2lib/util.hpp',
    subdir: 'cs2lib',
)

test(
    'cs2-lib',
    executable(
        'cs2-lib-test',
        src,
        'test.cpp',
        cpp_args: [
            '-DCS2LIB_FULL_FUZZ_DEPTH=1',
            '-DCS2LIB_CURATED_FUZZ_DEPTH=2',
        ],
        include_directories: inc_dir,
        dependencies: [cpptrace_dep, doctest_dep],
    ),
    suite: 'basic',
    timeout: 60,
)
test(
    'cs2-lib-test-fuzz',
    executable(
        'cs2-lib-fuzz',
        src,
        'test.cpp',
        cpp_args: [
            '-DCS2LIB_FULL_FUZZ_DEPTH=3',
            '-DCS2LIB_CURATED_FUZZ_DEPTH=3',
        ],
        include_directories: inc_dir,
        dependencies: [cpptrace_dep, doctest_dep],
    ),
    timeout: 0,
    suite: 'fuzz',
)
test(
    'cs2-lib-test-deep',
    executable(
        'cs2-lib-deep',
        src,
        'test.cpp',
        cpp_args: [
            '-DCS2LIB_FULL_FUZZ_DEPTH=4',
            '-DCS2LIB_CURATED_FUZZ_DEPTH=5',
        ],
        include_directories: inc_dir,
        dependencies: [cpptrace_dep, doctest_dep],
    ),
    timeout: 0,
    suite: 'deep',
)
cs2_lib_dep = declare_dependency(
    link_with: cs2_lib,
    include_directories: inc_dir,
)
import('pkgconfig').generate(
    cs2_lib,
    name: 'cs2-lib',
    description: 'My library for computer science 2 projects',
    version: '0.1.0',
)
