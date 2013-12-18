

#ifndef PHP_SOME_H
#define PHP_SOME_H 1

#include "kernel/globals.h"

#define PHP_SOME_VERSION "0.0.1"
#define PHP_SOME_EXTNAME "heka"



ZEND_BEGIN_MODULE_GLOBALS(heka)

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
	
ZEND_END_MODULE_GLOBALS(heka)

#ifdef ZTS
#include "TSRM.h"
#endif

ZEND_EXTERN_MODULE_GLOBALS(heka)

#ifdef ZTS
	#define ZEPHIR_GLOBAL(v) TSRMG(heka_globals_id, zend_heka_globals *, v)
#else
	#define ZEPHIR_GLOBAL(v) (heka_globals.v)
#endif

#ifdef ZTS
	#define ZEPHIR_VGLOBAL ((zend_heka_globals *) (*((void ***) tsrm_ls))[TSRM_UNSHUFFLE_RSRC_ID(heka_globals_id)])
#else
	#define ZEPHIR_VGLOBAL &(heka_globals)
#endif

#define zephir_globals heka_globals
#define zend_zephir_globals zend_heka_globals

extern zend_module_entry heka_module_entry;
#define phpext_heka_ptr &heka_module_entry

#endif
