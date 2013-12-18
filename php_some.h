

#ifndef PHP_SOME_H
#define PHP_SOME_H 1

#include "kernel/globals.h"

#define PHP_SOME_VERSION "0.0.1"
#define PHP_SOME_EXTNAME "some"



ZEND_BEGIN_MODULE_GLOBALS(some)

	/* Memory */
	zephir_memory_entry *start_memory;
	zephir_memory_entry *active_memory;

	/* Virtual Symbol Tables */
	zephir_symbol_table *active_symbol_table;

	/* Function cache */
	HashTable *function_cache;

	/* Max recursion control */
	unsigned int recursive_lock;

	/* Global constants */
	zval *global_true;
	zval *global_false;
	zval *global_null;
	
ZEND_END_MODULE_GLOBALS(some)

#ifdef ZTS
#include "TSRM.h"
#endif

ZEND_EXTERN_MODULE_GLOBALS(some)

#ifdef ZTS
	#define ZEPHIR_GLOBAL(v) TSRMG(some_globals_id, zend_some_globals *, v)
#else
	#define ZEPHIR_GLOBAL(v) (some_globals.v)
#endif

#ifdef ZTS
	#define ZEPHIR_VGLOBAL ((zend_some_globals *) (*((void ***) tsrm_ls))[TSRM_UNSHUFFLE_RSRC_ID(some_globals_id)])
#else
	#define ZEPHIR_VGLOBAL &(some_globals)
#endif

#define zephir_globals some_globals
#define zend_zephir_globals zend_some_globals

extern zend_module_entry some_module_entry;
#define phpext_some_ptr &some_module_entry

#endif
