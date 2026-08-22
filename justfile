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




BUILDDIR := env("BUILDDIR", 'build')

[arg('mode', help="The optimization mode to use")]
configure mode='debug':
	# THIS IS INTENTIONALLY UNQUOTED, THE ARGS ARE MEANT TO BE INTERPRETED AS A LIST.
	case '{{mode}}' in \
		"debug") export FLAGS="-Db_sanitize=address,undefined --optimization=0";; \
		"thread-debug") export FLAGS="-Db_sanitize=thread,undefined --optimization=1";; \
		"release") export FLAGS="--buildtype=release";; \
	esac && \
	meson setup '{{BUILDDIR}}' ${FLAGS}

[arg('mode', help="The optimization mode to use")]
build mode='debug': (configure mode)
	ninja -C '{{BUILDDIR}}'

[arg('executable', help="The executable to run")]
[arg('mode', help="The optimization mode to use")]
run executable mode='debug': (build mode)
	export EXE="$(echo "{{executable}}" | sed -E 's|([^-]*).*|\1/\0|')" && \
	"{{BUILDDIR}}/${EXE}"

[arg('executable', help="The executable to debug")]
[arg('mode', help="The optimization mode to use")]
debug executable mode='debug': (build mode)
	[[ "{{mode}}" == "release" ]] && echo >&2 "Cannot debug a release build" && exit 1
	export EXE="$(echo "{{executable}}" | sed -E 's|([^-]*).*|\1/\0|')" && \
	lldb "{{BUILDDIR}}/${EXE}"


[arg('mode', help="The optimization mode to use")]
install mode='release': (build mode)
	meson install

clean:
	rm -r '{{BUILDDIR}}'

