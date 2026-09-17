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

inc_dir = include_directories('include')
doctest_dep = dependency('doctest')
src = ['hash.cpp', 'sexp.cpp']

cs2_lib = static_library(
    'cs2-lib',
    src,
    include_directories: inc_dir,
    install: true,
    cpp_args: ['-DDOCTEST_CONFIG_DISABLE'],
    dependencies: [doctest_dep],
)
test(
    'cs2-lib',
    executable(
        'cs2-lib-test',
        src,
        include_directories: inc_dir,
        cpp_args: ['-DDOCTEST_CONFIG_IMPLEMENT_WITH_MAIN'],
        dependencies: [doctest_dep],
    ),
)
cs2_lib_dep = declare_dependency(
    link_with: cs2_lib,
    include_directories: inc_dir,
)
import('pkgconfig').generate(cs2_lib)
