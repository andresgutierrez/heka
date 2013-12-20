
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
LLVMValueRef msg, msg_ptr;

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

int heka_init_php_printf()
{
	LLVMTypeRef *arg_tys;
	LLVMValueRef function;

	/**
	 * Initialize php_printf for longs
	 */
	arg_tys    = malloc(2 * sizeof(*arg_tys));
	arg_tys[0] = LLVMPointerType(LLVMInt8Type(), 0);
	arg_tys[1] = LLVMInt32Type();
	function = LLVMAddFunction(module, "php_printf_long", LLVMFunctionType(LLVMVoidType(), arg_tys, 2, 0));
	if (!function) {
		zend_error_noreturn(E_ERROR, "Cannot register php_printf_long");
	}

	LLVMAddGlobalMapping(exec_engine, function, php_printf);
	LLVMSetFunctionCallConv(function, LLVMCCallConv);
	LLVMAddFunctionAttr(function, LLVMNoUnwindAttribute);

	/**
	 * Initialize php_printf for doubles
	 */
	arg_tys    = malloc(2 * sizeof(*arg_tys));
	arg_tys[0] = LLVMPointerType(LLVMInt8Type(), 0);
	arg_tys[1] = LLVMDoubleType();
	function = LLVMAddFunction(module, "php_printf_double", LLVMFunctionType(LLVMVoidType(), arg_tys, 2, 0));
	if (!function) {
		zend_error_noreturn(E_ERROR, "Cannot register php_printf_double");
	}

	LLVMAddGlobalMapping(exec_engine, function, php_printf);
	LLVMSetFunctionCallConv(function, LLVMCCallConv);
	LLVMAddFunctionAttr(function, LLVMNoUnwindAttribute);

	/**
	 * Initialize php_printf for strings
	 */
	arg_tys    = malloc(2 * sizeof(*arg_tys));
	arg_tys[0] = LLVMPointerType(LLVMInt8Type(), 0);
	arg_tys[1] = LLVMPointerType(LLVMInt8Type(), 0);
	function = LLVMAddFunction(module, "php_printf_string", LLVMFunctionType(LLVMVoidType(), arg_tys, 2, 0));
	if (!function) {
		zend_error_noreturn(E_ERROR, "Cannot register php_printf_string");
	}

	LLVMAddGlobalMapping(exec_engine, function, php_printf);
	LLVMSetFunctionCallConv(function, LLVMCCallConv);
	LLVMAddFunctionAttr(function, LLVMNoUnwindAttribute);

	return 0;
}

int heka_init_functions()
{
	heka_init_php_printf();
	return 0;
}

LLVMValueRef llvmGenLocalStringVar(const char* data, int len)
{

	LLVMValueRef glob = LLVMAddGlobal(module, LLVMArrayType(LLVMInt8Type(), len), "string");

	// set as internal linkage and constant
	LLVMSetLinkage(glob, LLVMInternalLinkage);
	LLVMSetGlobalConstant(glob, 1);

	// Initialize with string:
	LLVMSetInitializer(glob, LLVMConstString(data, len, 1));

	LLVMDumpType(LLVMTypeOf(glob));

	return glob;
}

int heka_init_jit_engine(){

	char *err;

	module = LLVMModuleCreateWithName("heka");
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
	/*LLVMAddPromoteMemoryToRegisterPass(pass_mgr);
	LLVMAddInstructionCombiningPass(pass_mgr);
	LLVMAddReassociatePass(pass_mgr);
	LLVMAddGVNPass(pass_mgr);*/
	LLVMAddCFGSimplificationPass(pass_mgr);
	LLVMInitializeFunctionPassManager(pass_mgr);

	heka_init_functions();

	return 0;
}

void heka_echo_func(zend_op* op TSRMLS_DC) {

	LLVMValueRef args[2];
	LLVMValueRef php_printf_func;
	zval *tmp_zval;

	if (op->op1.op_type == IS_CONST) {

		tmp_zval = &op->op1.u.constant;
		switch (Z_TYPE_P(tmp_zval)) {

			case IS_LONG:

				php_printf_func = LLVMGetNamedFunction(module, "php_printf_long");
				if (!php_printf_func) {
					zend_error_noreturn(E_ERROR, "Cannot load php_printf_long");
				}

				args[0] = LLVMBuildGlobalStringPtr(builder, "%d", "");
				args[1] = LLVMConstInt(LLVMInt32Type(), Z_LVAL_P(tmp_zval), 0);
				LLVMBuildCall(builder, php_printf_func, args, 2, "");
				break;

			case IS_DOUBLE:

				php_printf_func = LLVMGetNamedFunction(module, "php_printf_double");
				if (!php_printf_func) {
					zend_error_noreturn(E_ERROR, "Cannot load php_printf_double");
				}

				args[0] = LLVMBuildGlobalStringPtr(builder, "%f", "");
				args[1] = LLVMConstReal(LLVMDoubleType(), Z_DVAL_P(tmp_zval));
				LLVMBuildCall(builder, php_printf_func, args, 2, "");
				break;

			case IS_STRING:

				php_printf_func = LLVMGetNamedFunction(module, "php_printf_string");
				if (!php_printf_func) {
					zend_error_noreturn(E_ERROR, "Cannot load php_printf_string");
				}

				args[0] = LLVMBuildGlobalStringPtr(builder, "%s", "");
				args[1] = LLVMBuildGlobalStringPtr(builder, Z_STRVAL_P(tmp_zval), "");
				LLVMBuildCall(builder, php_printf_func, args, 2, "");
				break;
		}
	}

}

LLVMBasicBlockRef heka_create_label(LLVMValueRef function, int i, int use_name)
{
	char *label_name;
	LLVMBasicBlockRef branch_bb;

	if (use_name) {
		spprintf(&label_name, 0, "label_%d", i);
		branch_bb   = LLVMAppendBasicBlock(function, label_name);
	} else {
		branch_bb   = LLVMAppendBasicBlock(function, "");
	}

	LLVMPositionBuilderAtEnd(builder, branch_bb);
	LLVMBuildRetVoid(builder);

	return branch_bb;
}

LLVMValueRef heka_compile_func(zend_op_array *op_array, char* fn_name, LLVMModuleRef module, LLVMExecutionEngineRef exec_engine TSRMLS_DC)
{

	zend_uint i, j;
	LLVMValueRef function;
	LLVMBasicBlockRef basic_block;
	unsigned int jump_offset;
	zend_op *op, *op_dest;
	LLVMBasicBlockRef blocks[256];
	int referenced_blocks[256];
	znode *node;

	function = LLVMAddFunction(module, fn_name, LLVMFunctionType(LLVMVoidType(), NULL, 0, 0));

	for (i = 0; i < 256; i++) {
		referenced_blocks[i] = 0;
	}

	basic_block = LLVMAppendBasicBlock(function, "init");
	LLVMPositionBuilderAtEnd(builder, basic_block);
	LLVMBuildRetVoid(builder);

	for (i = 0; i < op_array->last; ++i) {

		op = op_array->opcodes + i;
		switch (op->opcode) {
			case ZEND_JMPZ:
			case ZEND_JMP:

				for (j = 0; j < op_array->last; ++j) {
					op_dest = op_array->opcodes + j;
					if (op->opcode == ZEND_JMPZ) {
						node = &op->op2;
					} else {
						node = &op->op1;
					}
					if (op_dest == (node->u.jmp_addr + 1)) {
						blocks[i] = heka_create_label(function, i, 1);
						referenced_blocks[j] = 1;
						break;
					}
				}
				break;
		}

	}

	for (i = 0; i < op_array->last; ++i) {

		op = op_array->opcodes + i;
		switch (op->opcode) {
			case ZEND_EXT_NOP:
			case ZEND_EXT_STMT:
				blocks[i] = NULL;
				continue;
		}

		switch (op->opcode) {
			case ZEND_JMPZ:
			case ZEND_JMP:
				break;
			case ZEND_ECHO:
				if (referenced_blocks[i]) {
					blocks[i] = heka_create_label(function, i, 1);
				}
				heka_echo_func(op);
				break;
			case ZEND_RETURN:
				if (referenced_blocks[i]) {
					blocks[i] = heka_create_label(function, i, 1);
				}
				LLVMBuildRetVoid(builder);
				break;
			default:
				fprintf(stderr, "%s\n", heka_get_op_name(op->opcode));
		}
	}

	for (i = 0; i < op_array->last; ++i) {

		op = op_array->opcodes + i;
		switch (op->opcode) {
			case ZEND_JMPZ:
			case ZEND_JMP:
				//jump_offset = ((char*) op->op2.u.var - (char*)op_array->opcodes) / sizeof(zend_op);
				//fprintf(stderr, "%s %u\n", heka_get_op_name((op->op2.u.jmp_addr + 1)->opcode), jump_offset);

				for (j = 0; j < op_array->last; ++j) {
					op_dest = op_array->opcodes + j;
					if (op->opcode == ZEND_JMPZ) {
						node = &op->op2;
					} else {
						node = &op->op1;
					}
					if (op_dest == (node->u.jmp_addr + 1)) {
						//fprintf(stderr, "%s %u\n", heka_get_op_name(op_dest->opcode), jump_offset);
						LLVMPositionBuilderAtEnd(builder, blocks[i]);
						LLVMBuildBr(builder, blocks[j]);
						break;
					}
				}
				break;
		}

	}

	/*basic_block = LLVMAppendBasicBlock(function, "");
	LLVMPositionBuilderAtEnd(builder, basic_block);
	LLVMBuildRetVoid(builder);*/

	return function;
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

	/**
	 * We're only interested in functions
	 */
	if (op_array->function_name) {

		function = LLVMGetNamedFunction(module, op_array->function_name);

		if (!function) {

			function = heka_compile_func(op_array, op_array->function_name, module, exec_engine TSRMLS_CC);
			if (!function) {
				zend_error_noreturn(E_ERROR, "Cannot compile function");
			}

			LLVMDumpValue(function);
		}

		LLVMRunFunction(exec_engine, function, 0, NULL);
		//LLVMRunFunctionPassManager();

	} else {
		old_execute(op_array TSRMLS_CC);
	}

	/*fprintf(name, "%s__c__%s__f__%s__s",
		(op_array->filename)? op_array->filename : "",
		(op_array->scope)? op_array->scope->name : "",
		(op_array->function_name)? op_array->function_name : "");*/

	//old_execute(op_array TSRMLS_CC);
}

ZEND_API void heka_execute_internal(zend_execute_data *execute_data_ptr, int return_value_used TSRMLS_DC)
{
	zend_execute_internal(execute_data_ptr, return_value_used);
}

static PHP_MSHUTDOWN_FUNCTION(heka){

	LLVMDumpModule(module);

	//hash_free (&symtbl);
	LLVMDisposePassManager (pass_mgr);
	LLVMDisposeBuilder (builder);
	LLVMDisposeModule (module);

	//assert(ZEPHIR_GLOBAL(function_cache) == NULL);

	return SUCCESS;
}

static PHP_RINIT_FUNCTION(heka){

	//php_zephir_init_globals(ZEPHIR_VGLOBAL TSRMLS_CC);
	//heka_init_interned_strings(TSRMLS_C);

	return SUCCESS;
}

static PHP_RSHUTDOWN_FUNCTION(heka){

	/*if (ZEPHIR_GLOBAL(start_memory) != NULL) {
		zephir_clean_restore_stack(TSRMLS_C);
	}

	if (ZEPHIR_GLOBAL(function_cache) != NULL) {
		zend_hash_destroy(ZEPHIR_GLOBAL(function_cache));
		FREE_HASHTABLE(ZEPHIR_GLOBAL(function_cache));
		ZEPHIR_GLOBAL(function_cache) = NULL;
	}*/

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
	//zephir_memory_entry *start;

	//php_zephir_init_globals(heka_globals TSRMLS_CC);

	/* Start Memory Frame */
	/*start = (zephir_memory_entry *) pecalloc(1, sizeof(zephir_memory_entry), 1);
	start->addresses       = pecalloc(24, sizeof(zval*), 1);
	start->capacity        = 24;
	start->hash_addresses  = pecalloc(8, sizeof(zval*), 1);
	start->hash_capacity   = 8;

	heka_globals->start_memory = start;*/

	/* Global Constants */
	/*ALLOC_PERMANENT_ZVAL(heka_globals->global_false);
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
	Z_ADDREF_P(heka_globals->global_null);*/
}

static PHP_GSHUTDOWN_FUNCTION(heka)
{
	/*assert(heka_globals->start_memory != NULL);

	pefree(heka_globals->start_memory->hash_addresses, 1);
	pefree(heka_globals->start_memory->addresses, 1);
	pefree(heka_globals->start_memory, 1);
	heka_globals->start_memory = NULL;*/
}

zend_module_entry heka_module_entry = {
	STANDARD_MODULE_HEADER_EX,
	NULL,
	NULL,
	PHP_HEKA_EXTNAME,
	NULL,
	PHP_MINIT(heka),
	PHP_MSHUTDOWN(heka),
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

