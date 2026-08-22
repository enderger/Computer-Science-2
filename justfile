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

[arg('mode', help="The optimization mode to use")]
install mode='release': (build mode)
	meson install

clean:
	rm -r '{{BUILDDIR}}'

