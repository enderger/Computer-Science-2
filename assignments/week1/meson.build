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
doctest_dep = dependency('doctest')
incdir = include_directories('include')

executable('week1-day1', 'week1-day1.cpp', install: true)

test('week1-day2',
  executable('week1-day2-tests', 'week1-day2.cpp',
    cpp_args: ['-DDOCTEST_CONFIG_IMPLEMENT_WITH_MAIN'],
    dependencies: doctest_dep,
    include_directories: incdir))

executable('week1-day2', 'week1-day2.cpp', 'week1-day2-runner.cpp',
    cpp_args: ['-DDOCTEST_CONFIG_DISABLE'],
    dependencies: doctest_dep,
    include_directories: incdir,
    install: true)
