
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <php.h>
#include "php_ext.h"
#include "some.h"

#include <ext/standard/info.h>

#include <Zend/zend_operators.h>
#include <Zend/zend_exceptions.h>
#include <Zend/zend_interfaces.h>

#include <llvm-c/Analysis.h>
#include <llvm-c/Target.h>
#include <llvm-c/Transforms/Scalar.h>
#include <llvm-c/ExecutionEngine.h>

#include "kernel/main.h"
#include "kernel/memory.h"

ZEND_DLEXPORT void (*old_execute)(zend_op_array *op_array TSRMLS_DC);
ZEND_DLEXPORT void some_execute(zend_op_array *op_array TSRMLS_DC);
ZEND_DLEXPORT void some_execute_internal(zend_execute_data *execute_data_ptr, int return_value_used TSRMLS_DC);

zend_class_entry *some_main_ce;

LLVMModuleRef module;
LLVMBuilderRef builder;
LLVMPassManagerRef pass_mgr;
LLVMExecutionEngineRef exec_engine;

ZEND_DECLARE_MODULE_GLOBALS(some);

int some_init_jit_engine(){

	char *err;

	module = LLVMModuleCreateWithName("kaleidoscope JIT");
	builder = LLVMCreateBuilder();

	LLVMInitializeNativeTarget();
	LLVMLinkInJIT();

	/*
	if (!LLVMCreateJITCompilerForModule (&exec_engine, module,
				LLVM_CODE_GEN_OPT_LEVEL_DEFAULT, &err)) {
			*/
	if (LLVMCreateExecutionEngineForModule (&exec_engine, module, &err)) {
		fprintf (stderr, "error: %s\n", err);
		LLVMDisposeMessage (err);
		return 1;
	}

	pass_mgr =  LLVMCreateFunctionPassManagerForModule (module);
	LLVMAddTargetData (LLVMGetExecutionEngineTargetData (exec_engine), pass_mgr);
	LLVMAddPromoteMemoryToRegisterPass (pass_mgr);
	LLVMAddInstructionCombiningPass (pass_mgr);
	LLVMAddReassociatePass (pass_mgr);
	LLVMAddGVNPass (pass_mgr);
	LLVMAddCFGSimplificationPass (pass_mgr);
	LLVMInitializeFunctionPassManager (pass_mgr);

	return 0;
}

PHP_MINIT_FUNCTION(some){

	ZEPHIR_INIT(Some_Main);

	some_init_jit_engine();

	old_execute = zend_execute;
	zend_execute = some_execute;
	zend_execute_internal = some_execute_internal;

	return SUCCESS;
}

ZEND_API void some_execute(zend_op_array *op_array TSRMLS_DC)
{
	//char *fname = NULL;
	char* name;

	fprintf(, 0, "%s__c__%s__f__%s__s",
		(op_array->filename)? op_array->filename : "",
		(op_array->scope)? op_array->scope->name : "",
		(op_array->function_name)? op_array->function_name : "");

	old_execute(op_array TSRMLS_CC);
}

ZEND_API void some_execute_internal(zend_execute_data *execute_data_ptr, int return_value_used TSRMLS_DC)
{
	//char *fname = NULL;
	//zend_execute_data *execd;

	zend_execute_internal(execute_data_ptr, return_value_used);
}

static PHP_MSHUTDOWN_FUNCTION(some){

	LLVMDumpModule (module);

	//hash_free (&symtbl);
	LLVMDisposePassManager (pass_mgr);
	LLVMDisposeBuilder (builder);
	LLVMDisposeModule (module);

	assert(ZEPHIR_GLOBAL(function_cache) == NULL);

	return SUCCESS;
}

static PHP_RINIT_FUNCTION(some){

	php_zephir_init_globals(ZEPHIR_VGLOBAL TSRMLS_CC);
	//some_init_interned_strings(TSRMLS_C);

	return SUCCESS;
}

static PHP_RSHUTDOWN_FUNCTION(some){

	if (ZEPHIR_GLOBAL(start_memory) != NULL) {
		zephir_clean_restore_stack(TSRMLS_C);
	}

	if (ZEPHIR_GLOBAL(function_cache) != NULL) {
		zend_hash_destroy(ZEPHIR_GLOBAL(function_cache));
		FREE_HASHTABLE(ZEPHIR_GLOBAL(function_cache));
		ZEPHIR_GLOBAL(function_cache) = NULL;
	}

	return SUCCESS;
}

static PHP_MINFO_FUNCTION(some)
{
	php_info_print_table_start();
	php_info_print_table_row(2, "Version", PHP_SOME_VERSION);
	php_info_print_table_end();
}

static PHP_GINIT_FUNCTION(some)
{
	zephir_memory_entry *start;

	php_zephir_init_globals(some_globals TSRMLS_CC);

	/* Start Memory Frame */
	start = (zephir_memory_entry *) pecalloc(1, sizeof(zephir_memory_entry), 1);
	start->addresses       = pecalloc(24, sizeof(zval*), 1);
	start->capacity        = 24;
	start->hash_addresses  = pecalloc(8, sizeof(zval*), 1);
	start->hash_capacity   = 8;

	some_globals->start_memory = start;

	/* Global Constants */
	ALLOC_PERMANENT_ZVAL(some_globals->global_false);
	INIT_PZVAL(some_globals->global_false);
	ZVAL_FALSE(some_globals->global_false);
	Z_ADDREF_P(some_globals->global_false);

	ALLOC_PERMANENT_ZVAL(some_globals->global_true);
	INIT_PZVAL(some_globals->global_true);
	ZVAL_TRUE(some_globals->global_true);
	Z_ADDREF_P(some_globals->global_true);

	ALLOC_PERMANENT_ZVAL(some_globals->global_null);
	INIT_PZVAL(some_globals->global_null);
	ZVAL_NULL(some_globals->global_null);
	Z_ADDREF_P(some_globals->global_null);
}

static PHP_GSHUTDOWN_FUNCTION(some)
{
	assert(some_globals->start_memory != NULL);

	pefree(some_globals->start_memory->hash_addresses, 1);
	pefree(some_globals->start_memory->addresses, 1);
	pefree(some_globals->start_memory, 1);
	some_globals->start_memory = NULL;
}

zend_module_entry some_module_entry = {
	STANDARD_MODULE_HEADER_EX,
	NULL,
	NULL,
	PHP_SOME_EXTNAME,
	NULL,
	PHP_MINIT(some),
#ifndef ZEPHIR_RELEASE
	PHP_MSHUTDOWN(some),
#else
	NULL,
#endif
	PHP_RINIT(some),
	PHP_RSHUTDOWN(some),
	PHP_MINFO(some),
	PHP_SOME_VERSION,
	ZEND_MODULE_GLOBALS(some),
	PHP_GINIT(some),
	PHP_GSHUTDOWN(some),
	NULL,
	STANDARD_MODULE_PROPERTIES_EX
};

#ifdef COMPILE_DL_SOME
ZEND_GET_MODULE(some)
#endif

