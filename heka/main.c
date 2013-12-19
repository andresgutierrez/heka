
#ifdef HAVE_CONFIG_H
#include "../ext_config.h"
#endif

#include <php.h>
#include "../php_ext.h"
#include "../ext.h"

#include <Zend/zend_operators.h>
#include <Zend/zend_exceptions.h>
#include <Zend/zend_interfaces.h>

#include "kernel/main.h"

ZEPHIR_INIT_CLASS(Heka_Main) {
	ZEPHIR_REGISTER_CLASS(Heka, Main, heka, main, NULL, 0);
	return SUCCESS;
}

