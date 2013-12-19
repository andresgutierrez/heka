
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <php.h>
#include "php_ext.h"
#include "heka.h"

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

typedef struct _heka_opcode_list {
	long val;
	char *name;
	long flags;
} heka_opcode_list;

static heka_opcode_list opcode_names[] = {
	{ ZEND_NOP,						"ZEND_NOP",						0 },
	{ ZEND_ADD,						"ZEND_ADD",						0 },
	{ ZEND_SUB,						"ZEND_SUB",						0 },
	{ ZEND_MUL,						"ZEND_MUL",						0 },
	{ ZEND_DIV,						"ZEND_DIV",						0 },
	{ ZEND_MOD,						"ZEND_MOD",						0 },
	{ ZEND_SL,						"ZEND_SL",						0 },
	{ ZEND_SR,						"ZEND_SR",						0 },
	{ ZEND_CONCAT,					"ZEND_CONCAT",					0 },
	{ ZEND_BW_OR,					"ZEND_BW_OR",					0 },
	{ ZEND_BW_AND,					"ZEND_BW_AND",					0 },
	{ ZEND_BW_XOR,					"ZEND_BW_XOR",					0 },
	{ ZEND_BW_NOT,					"ZEND_BW_NOT",					0 },
	{ ZEND_BOOL_NOT,				"ZEND_BOOL_NOT",				0 },
	{ ZEND_BOOL_XOR,				"ZEND_BOOL_XOR",				0 },
	{ ZEND_IS_IDENTICAL,			"ZEND_IS_IDENTICAL",			0 },
	{ ZEND_IS_NOT_IDENTICAL,		"ZEND_IS_NOT_IDENTICAL",		0 },
	{ ZEND_IS_EQUAL,				"ZEND_IS_EQUAL",				0 },
	{ ZEND_IS_NOT_EQUAL,			"ZEND_IS_NOT_EQUAL",			0 },
	{ ZEND_IS_SMALLER,				"ZEND_IS_SMALLER",				0 },
	{ ZEND_IS_SMALLER_OR_EQUAL,		"ZEND_IS_SMALLER_OR_EQUAL",		0 },
	{ ZEND_CAST,					"ZEND_CAST",					0 },
	{ ZEND_QM_ASSIGN,				"ZEND_QM_ASSIGN",				0 },
	{ ZEND_ASSIGN_ADD,				"ZEND_ASSIGN_ADD",				0 },
	{ ZEND_ASSIGN_SUB,				"ZEND_ASSIGN_SUB",				0 },
	{ ZEND_ASSIGN_MUL,				"ZEND_ASSIGN_MUL",				0 },
	{ ZEND_ASSIGN_DIV,				"ZEND_ASSIGN_DIV",				0 },
	{ ZEND_ASSIGN_MOD,				"ZEND_ASSIGN_MOD",				0 },
	{ ZEND_ASSIGN_SL,				"ZEND_ASSIGN_SL",				0 },
	{ ZEND_ASSIGN_SR,				"ZEND_ASSIGN_SR",				0 },
	{ ZEND_ASSIGN_CONCAT,			"ZEND_ASSIGN_CONCAT",			0 },
	{ ZEND_ASSIGN_BW_OR,			"ZEND_ASSIGN_BW_OR",			0 },
	{ ZEND_ASSIGN_BW_AND,			"ZEND_ASSIGN_BW_AND",			0 },
	{ ZEND_ASSIGN_BW_XOR,			"ZEND_ASSIGN_BW_XOR",			0 },
	{ ZEND_PRE_INC,					"ZEND_PRE_INC",					0 },
	{ ZEND_PRE_DEC,					"ZEND_PRE_DEC",					0 },
	{ ZEND_POST_INC,				"ZEND_POST_INC",				0 },
	{ ZEND_POST_DEC,				"ZEND_POST_DEC",				0 },
	{ ZEND_ASSIGN,					"ZEND_ASSIGN",					0 },
	{ ZEND_ASSIGN_REF,				"ZEND_ASSIGN_REF",				0 },
	{ ZEND_ECHO,					"ZEND_ECHO",					0 },
	{ ZEND_PRINT,					"ZEND_PRINT",					0 },
	{ ZEND_JMP,						"ZEND_JMP",						0 },
	{ ZEND_JMPZ,					"ZEND_JMPZ",					0 },
	{ ZEND_JMPNZ,					"ZEND_JMPNZ",					0 },
	{ ZEND_JMPZNZ,					"ZEND_JMPZNZ",					0 },
	{ ZEND_JMPZ_EX,					"ZEND_JMPZ_EX",					0 },
	{ ZEND_JMPNZ_EX,				"ZEND_JMPNZ_EX",				0 },
	{ ZEND_CASE,					"ZEND_CASE",					0 },
	{ ZEND_SWITCH_FREE,				"ZEND_SWITCH_FREE",				0 },
	{ ZEND_BRK,						"ZEND_BRK",						0 },
	{ ZEND_CONT,					"ZEND_CONT",					0 },
	{ ZEND_BOOL,					"ZEND_BOOL",					0 },
	{ ZEND_INIT_STRING,				"ZEND_INIT_STRING",				0 },
	{ ZEND_ADD_CHAR,				"ZEND_ADD_CHAR",				0 },
	{ ZEND_ADD_STRING,				"ZEND_ADD_STRING",				0 },
	{ ZEND_ADD_VAR,					"ZEND_ADD_VAR",					0 },
	{ ZEND_BEGIN_SILENCE,			"ZEND_BEGIN_SILENCE",			0 },
	{ ZEND_END_SILENCE,				"ZEND_END_SILENCE",				0 },
	{ ZEND_INIT_FCALL_BY_NAME,		"ZEND_INIT_FCALL_BY_NAME",		0 },
	{ ZEND_DO_FCALL,				"ZEND_DO_FCALL",				0 },
	{ ZEND_DO_FCALL_BY_NAME,		"ZEND_DO_FCALL_BY_NAME",		0 },
	{ ZEND_RETURN,					"ZEND_RETURN",					0 },
	{ ZEND_RECV,					"ZEND_RECV",					0 },
	{ ZEND_RECV_INIT,				"ZEND_RECV_INIT",				0 },
	{ ZEND_SEND_VAL,				"ZEND_SEND_VAL",				0 },
	{ ZEND_SEND_VAR,				"ZEND_SEND_VAR",				0 },
	{ ZEND_SEND_REF,				"ZEND_SEND_REF",				0 },
	{ ZEND_NEW,						"ZEND_NEW",						0 },
	{ ZEND_FREE,					"ZEND_FREE",					0 },
	{ ZEND_INIT_ARRAY,				"ZEND_INIT_ARRAY",				0 },
	{ ZEND_ADD_ARRAY_ELEMENT,		"ZEND_ADD_ARRAY_ELEMENT",		0 },
	{ ZEND_INCLUDE_OR_EVAL,			"ZEND_INCLUDE_OR_EVAL",			0 },
	{ ZEND_UNSET_VAR,				"ZEND_UNSET_VAR",				0 },
	{ ZEND_FE_RESET,				"ZEND_FE_RESET",				0 },
	{ ZEND_FE_FETCH,				"ZEND_FE_FETCH",				0 },
	{ ZEND_EXIT,					"ZEND_EXIT",					0 },
	{ ZEND_FETCH_R,					"ZEND_FETCH_R",					0 },
	{ ZEND_FETCH_DIM_R,				"ZEND_FETCH_DIM_R",				0 },
	{ ZEND_FETCH_OBJ_R,				"ZEND_FETCH_OBJ_R",				0 },
	{ ZEND_FETCH_W,					"ZEND_FETCH_W",					0 },
	{ ZEND_FETCH_DIM_W,				"ZEND_FETCH_DIM_W",				0 },
	{ ZEND_FETCH_OBJ_W,				"ZEND_FETCH_OBJ_W",				0 },
	{ ZEND_FETCH_RW,				"ZEND_FETCH_RW",				0 },
	{ ZEND_FETCH_DIM_RW,			"ZEND_FETCH_DIM_RW",			0 },
	{ ZEND_FETCH_OBJ_RW,			"ZEND_FETCH_OBJ_RW",			0 },
	{ ZEND_FETCH_IS,				"ZEND_FETCH_IS",				0 },
	{ ZEND_FETCH_DIM_IS,			"ZEND_FETCH_DIM_IS",			0 },
	{ ZEND_FETCH_OBJ_IS,			"ZEND_FETCH_OBJ_IS",			0 },
	{ ZEND_FETCH_FUNC_ARG,			"ZEND_FETCH_FUNC_ARG",			0 },
	{ ZEND_FETCH_DIM_FUNC_ARG,		"ZEND_FETCH_DIM_FUNC_ARG",		0 },
	{ ZEND_FETCH_OBJ_FUNC_ARG,		"ZEND_FETCH_OBJ_FUNC_ARG",		0 },
	{ ZEND_FETCH_UNSET,				"ZEND_FETCH_UNSET",				0 },
	{ ZEND_FETCH_DIM_UNSET,			"ZEND_FETCH_DIM_UNSET",			0 },
	{ ZEND_FETCH_OBJ_UNSET,			"ZEND_FETCH_OBJ_UNSET",			0 },
	{ ZEND_FETCH_DIM_TMP_VAR,		"ZEND_FETCH_DIM_TMP_VAR",		0 },
	{ ZEND_FETCH_CONSTANT,			"ZEND_FETCH_CONSTANT",			0 },
	{ ZEND_EXT_STMT,				"ZEND_EXT_STMT",				0 },
	{ ZEND_EXT_FCALL_BEGIN,			"ZEND_EXT_FCALL_BEGIN",			0 },
	{ ZEND_EXT_FCALL_END,			"ZEND_EXT_FCALL_END",			0 },
	{ ZEND_EXT_NOP,					"ZEND_EXT_NOP",					0 },
	{ ZEND_TICKS,					"ZEND_TICKS",					0 },
	{ ZEND_SEND_VAR_NO_REF,			"ZEND_SEND_VAR_NO_REF",			0 },
	{ ZEND_CATCH,					"ZEND_CATCH",					0 },
	{ ZEND_THROW,					"ZEND_THROW",					0 },
	{ ZEND_FETCH_CLASS,				"ZEND_FETCH_CLASS",				0 },
	{ ZEND_CLONE,					"ZEND_CLONE",					0 },
	{ ZEND_INIT_METHOD_CALL,		"ZEND_INIT_METHOD_CALL",		0 },
	{ ZEND_INIT_STATIC_METHOD_CALL,	"ZEND_INIT_STATIC_METHOD_CALL",	0 },
	{ ZEND_ISSET_ISEMPTY_VAR,		"ZEND_ISSET_ISEMPTY_VAR",		0 },
	{ ZEND_ISSET_ISEMPTY_DIM_OBJ,	"ZEND_ISSET_ISEMPTY_DIM_OBJ",	0 },
	{ ZEND_PRE_INC_OBJ,				"ZEND_PRE_INC_OBJ",				0 },
	{ ZEND_PRE_DEC_OBJ,				"ZEND_PRE_DEC_OBJ",				0 },
	{ ZEND_POST_INC_OBJ,			"ZEND_POST_INC_OBJ",			0 },
	{ ZEND_POST_DEC_OBJ,			"ZEND_POST_DEC_OBJ",			0 },
	{ ZEND_ASSIGN_OBJ,				"ZEND_ASSIGN_OBJ",				0 },
	{ ZEND_OP_DATA,					"ZEND_OP_DATA",					0 },
	{ ZEND_INSTANCEOF,				"ZEND_INSTANCEOF",				0 },
	{ ZEND_DECLARE_CLASS,			"ZEND_DECLARE_CLASS",			0 },
	{ ZEND_DECLARE_INHERITED_CLASS,	"ZEND_DECLARE_INHERITED_CLASS",	0 },
	{ ZEND_DECLARE_FUNCTION,		"ZEND_DECLARE_FUNCTION",		0 },
	{ ZEND_RAISE_ABSTRACT_ERROR,	"ZEND_RAISE_ABSTRACT_ERROR",	0 },
	{ ZEND_ADD_INTERFACE,			"ZEND_ADD_INTERFACE",			0 },
	{ ZEND_VERIFY_ABSTRACT_CLASS,	"ZEND_VERIFY_ABSTRACT_CLASS",	0 },
	{ ZEND_ASSIGN_DIM,				"ZEND_ASSIGN_DIM",				0 },
	{ ZEND_ISSET_ISEMPTY_PROP_OBJ,	"ZEND_ISSET_ISEMPTY_PROP_OBJ",	0 },
	{ ZEND_HANDLE_EXCEPTION,		"ZEND_HANDLE_EXCEPTION",		0 },
	{ 0, NULL }
};

ZEND_DLEXPORT void (*old_execute)(zend_op_array *op_array TSRMLS_DC);
ZEND_DLEXPORT void heka_execute(zend_op_array *op_array TSRMLS_DC);
ZEND_DLEXPORT void heka_execute_internal(zend_execute_data *execute_data_ptr, int return_value_used TSRMLS_DC);

zend_class_entry *heka_main_ce;

LLVMModuleRef module;
LLVMBuilderRef builder;
LLVMPassManagerRef pass_mgr;
LLVMExecutionEngineRef exec_engine;

ZEND_DECLARE_MODULE_GLOBALS(heka);

char *heka_get_op_name(zend_uint opcode TSRMLS_DC){
	heka_opcode_list *opcodes = opcode_names;
	while (opcodes) {
		if (opcodes->val == opcode) {
			return opcodes->name;
		}
		opcodes++;
	}
	return NULL;
}

int heka_init_jit_engine(){

	char *err;

	module = LLVMModuleCreateWithName("kaleidoscope JIT");
	builder = LLVMCreateBuilder();

	LLVMInitializeNativeTarget();
	LLVMLinkInJIT();

	/*
	if (!LLVMCreateJITCompilerForModule (&exec_engine, module,
				LLVM_CODE_GEN_OPT_LEVEL_DEFAULT, &err)) {
			*/
	if (LLVMCreateExecutionEngineForModule(&exec_engine, module, &err)) {
		fprintf(stderr, "error: %s\n", err);
		LLVMDisposeMessage(err);
		return 1;
	}

	pass_mgr =  LLVMCreateFunctionPassManagerForModule(module);
	LLVMAddTargetData(LLVMGetExecutionEngineTargetData(exec_engine), pass_mgr);
	LLVMAddPromoteMemoryToRegisterPass(pass_mgr);
	LLVMAddInstructionCombiningPass(pass_mgr);
	LLVMAddReassociatePass(pass_mgr);
	LLVMAddGVNPass(pass_mgr);
	LLVMAddCFGSimplificationPass(pass_mgr);
	LLVMInitializeFunctionPassManager(pass_mgr);

	return 0;
}

LLVMValueRef heka_compile_func(zend_op_array *op_array, char* fn_name, LLVMModuleRef module, LLVMExecutionEngineRef exec_engine TSRMLS_DC) {

	zend_uint i;
	LLVMValueRef function;

	function = LLVMGetBasicBlockParent(LLVMGetInsertBlock(builder));

	for (i = 0; i < op_array->last; ++i) {
		zend_op* op = op_array->opcodes + i;
		switch (op->opcode) {
			case ZEND_EXT_NOP:
			case ZEND_EXT_STMT:
				continue;
			case ZEND_ECHO:
				break;
		}

		fprintf(stderr, "%s\n", heka_get_op_name(op->opcode));
	}

}

PHP_MINIT_FUNCTION(heka){

	ZEPHIR_INIT(Heka_Main);

	heka_init_jit_engine();

	old_execute = zend_execute;
	zend_execute = heka_execute;
	zend_execute_internal = heka_execute_internal;

	return SUCCESS;
}

ZEND_API void heka_execute(zend_op_array *op_array TSRMLS_DC)
{
	LLVMValueRef function;
	char* name;

	/**
	 * We're only interested in functions
	 */
	if (op_array->function_name) {

		function = LLVMGetNamedFunction(module, op_array->function_name);

		if (!function) {
			function = heka_compile_func(op_array, op_array->function_name, module, exec_engine TSRMLS_CC);
			if (!function) {

			}
		}

	}

	/*fprintf(name, "%s__c__%s__f__%s__s",
		(op_array->filename)? op_array->filename : "",
		(op_array->scope)? op_array->scope->name : "",
		(op_array->function_name)? op_array->function_name : "");*/

	old_execute(op_array TSRMLS_CC);
}

ZEND_API void heka_execute_internal(zend_execute_data *execute_data_ptr, int return_value_used TSRMLS_DC)
{
	//char *fname = NULL;
	//zend_execute_data *execd;

	zend_execute_internal(execute_data_ptr, return_value_used);
}

static PHP_MSHUTDOWN_FUNCTION(heka){

	LLVMDumpModule (module);

	//hash_free (&symtbl);
	LLVMDisposePassManager (pass_mgr);
	LLVMDisposeBuilder (builder);
	LLVMDisposeModule (module);

	assert(ZEPHIR_GLOBAL(function_cache) == NULL);

	return SUCCESS;
}

static PHP_RINIT_FUNCTION(heka){

	php_zephir_init_globals(ZEPHIR_VGLOBAL TSRMLS_CC);
	//heka_init_interned_strings(TSRMLS_C);

	return SUCCESS;
}

static PHP_RSHUTDOWN_FUNCTION(heka){

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

static PHP_MINFO_FUNCTION(heka)
{
	php_info_print_table_start();
	php_info_print_table_row(2, "Version", PHP_HEKA_VERSION);
	php_info_print_table_end();
}

static PHP_GINIT_FUNCTION(heka)
{
	zephir_memory_entry *start;

	php_zephir_init_globals(heka_globals TSRMLS_CC);

	/* Start Memory Frame */
	start = (zephir_memory_entry *) pecalloc(1, sizeof(zephir_memory_entry), 1);
	start->addresses       = pecalloc(24, sizeof(zval*), 1);
	start->capacity        = 24;
	start->hash_addresses  = pecalloc(8, sizeof(zval*), 1);
	start->hash_capacity   = 8;

	heka_globals->start_memory = start;

	/* Global Constants */
	ALLOC_PERMANENT_ZVAL(heka_globals->global_false);
	INIT_PZVAL(heka_globals->global_false);
	ZVAL_FALSE(heka_globals->global_false);
	Z_ADDREF_P(heka_globals->global_false);

	ALLOC_PERMANENT_ZVAL(heka_globals->global_true);
	INIT_PZVAL(heka_globals->global_true);
	ZVAL_TRUE(heka_globals->global_true);
	Z_ADDREF_P(heka_globals->global_true);

	ALLOC_PERMANENT_ZVAL(heka_globals->global_null);
	INIT_PZVAL(heka_globals->global_null);
	ZVAL_NULL(heka_globals->global_null);
	Z_ADDREF_P(heka_globals->global_null);
}

static PHP_GSHUTDOWN_FUNCTION(heka)
{
	assert(heka_globals->start_memory != NULL);

	pefree(heka_globals->start_memory->hash_addresses, 1);
	pefree(heka_globals->start_memory->addresses, 1);
	pefree(heka_globals->start_memory, 1);
	heka_globals->start_memory = NULL;
}

zend_module_entry heka_module_entry = {
	STANDARD_MODULE_HEADER_EX,
	NULL,
	NULL,
	PHP_HEKA_EXTNAME,
	NULL,
	PHP_MINIT(heka),
#ifndef ZEPHIR_RELEASE
	PHP_MSHUTDOWN(heka),
#else
	NULL,
#endif
	PHP_RINIT(heka),
	PHP_RSHUTDOWN(heka),
	PHP_MINFO(heka),
	PHP_HEKA_VERSION,
	ZEND_MODULE_GLOBALS(heka),
	PHP_GINIT(heka),
	PHP_GSHUTDOWN(heka),
	NULL,
	STANDARD_MODULE_PROPERTIES_EX
};

#ifdef COMPILE_DL_HEKA
ZEND_GET_MODULE(heka)
#endif

