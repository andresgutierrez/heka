PHP_ARG_ENABLE(some, whether to enable some, [ --enable-some   Enable Some])

if test "$PHP_SOME" = "yes"; then
	AC_DEFINE(HAVE_SOME, 1, [Whether you have Some])
	some_sources="some.c kernel/main.c kernel/memory.c kernel/exception.c kernel/hash.c kernel/debug.c kernel/backtrace.c kernel/object.c kernel/array.c kernel/string.c kernel/fcall.c kernel/alternative/fcall.c kernel/file.c kernel/operators.c kernel/concat.c some/main.c"
	PHP_NEW_EXTENSION(some, $some_sources, $ext_shared)
fi
