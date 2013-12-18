PHP_ARG_ENABLE(heka, whether to enable heka, [ --enable-heka   Enable Some])

if test "$PHP_SOME" = "yes"; then
	AC_DEFINE(HAVE_SOME, 1, [Whether you have Some])
	heka_sources="heka.c kernel/main.c kernel/memory.c kernel/exception.c kernel/hash.c kernel/debug.c kernel/backtrace.c kernel/object.c kernel/array.c kernel/string.c kernel/fcall.c kernel/alternative/fcall.c kernel/file.c kernel/operators.c kernel/concat.c heka/main.c"
	PHP_NEW_EXTENSION(heka, $heka_sources, $ext_shared)
fi
