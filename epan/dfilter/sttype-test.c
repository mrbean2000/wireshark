/*
 * Wireshark - Network traffic analyzer
 * By Gerald Combs <gerald@wireshark.org>
 * Copyright 2001 Gerald Combs
 *
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "syntax-tree.h"
#include "sttype-test.h"

typedef struct {
	guint32		magic;
	test_op_t	op;
	stnode_t	*val1;
	stnode_t	*val2;
} test_t;

#define TEST_MAGIC	0xab9009ba

static gpointer
test_new(gpointer junk)
{
	test_t *test;

	g_assert_true(junk == NULL);

	test = g_new(test_t, 1);

	test->magic = TEST_MAGIC;
	test->op = TEST_OP_UNINITIALIZED;
	test->val1 = NULL;
	test->val2 = NULL;

	return test;
}

static gpointer
test_dup(gconstpointer data)
{
	const test_t *org = data;
	test_t *test;

	test = test_new(NULL);
	test->op   = org->op;
	test->val1 = stnode_dup(org->val1);
	test->val2 = stnode_dup(org->val1);

	return test;
}

static void
test_free(gpointer value)
{
	test_t *test = value;
	ws_assert_magic(test, TEST_MAGIC);

	if (test->val1)
		stnode_free(test->val1);
	if (test->val2)
		stnode_free(test->val2);

	g_free(test);
}

static char *
test_tostr(const void *value, gboolean pretty)
{
	const test_t *test = value;
	ws_assert_magic(test, TEST_MAGIC);

	if (pretty)
		return g_strdup(sttype_test_todisplay(test->op));

	const char *s = "<null>";

	switch(test->op) {
		case TEST_OP_EXISTS:
			s = "TEST_EXISTS";
			break;
		case TEST_OP_NOT:
			s = "TEST_NOT";
			break;
		case TEST_OP_AND:
			s = "TEST_AND";
			break;
		case TEST_OP_OR:
			s = "TEST_OR";
			break;
		case TEST_OP_ANY_EQ:
			s = "TEST_ANY_EQ";
			break;
		case TEST_OP_ALL_NE:
			s = "TEST_ALL_NE";
			break;
		case TEST_OP_ANY_NE:
			s = "TEST_ANY_NE";
			break;
		case TEST_OP_GT:
			s = "TEST_GT";
			break;
		case TEST_OP_GE:
			s = "TEST_GE";
			break;
		case TEST_OP_LT:
			s = "TEST_LT";
			break;
		case TEST_OP_LE:
			s = "TEST_LE";
			break;
		case TEST_OP_BITWISE_AND:
			s = "TEST_BITAND";
			break;
		case TEST_OP_CONTAINS:
			s = "TEST_CONTAINS";
			break;
		case TEST_OP_MATCHES:
			s = "TEST_MATCHES";
			break;
		case TEST_OP_IN:
			s = "TEST_IN";
			break;
		case TEST_OP_UNINITIALIZED:
			s = "<uninitialized>";
			break;
		default:
			break;
	}
	return g_strdup(s);
}

static int
num_operands(test_op_t op)
{
	switch(op) {
		case TEST_OP_UNINITIALIZED:
			break;
		case TEST_OP_EXISTS:
		case TEST_OP_NOT:
			return 1;
		case TEST_OP_AND:
		case TEST_OP_OR:
		case TEST_OP_ANY_EQ:
		case TEST_OP_ALL_NE:
		case TEST_OP_ANY_NE:
		case TEST_OP_GT:
		case TEST_OP_GE:
		case TEST_OP_LT:
		case TEST_OP_LE:
		case TEST_OP_BITWISE_AND:
		case TEST_OP_CONTAINS:
		case TEST_OP_MATCHES:
		case TEST_OP_IN:
			return 2;
	}
	g_assert_not_reached();
	return -1;
}


void
sttype_test_set1(stnode_t *node, test_op_t op, stnode_t *val1)
{
	test_t *test = stnode_data(node);
	ws_assert_magic(test, TEST_MAGIC);

	g_assert_true(num_operands(op) == 1);
	test->op = op;
	test->val1 = val1;
}

void
sttype_test_set2(stnode_t *node, test_op_t op, stnode_t *val1, stnode_t *val2)
{
	test_t *test = stnode_data(node);
	ws_assert_magic(test, TEST_MAGIC);

	g_assert_true(num_operands(op) == 2);
	test->op = op;
	test->val1 = val1;
	test->val2 = val2;
}

void
sttype_test_set2_args(stnode_t *node, stnode_t *val1, stnode_t *val2)
{
	test_t	*test;

	test = (test_t*)stnode_data(node);
	ws_assert_magic(test, TEST_MAGIC);

	if (num_operands(test->op) == 1) {
		g_assert_true(val2 == NULL);
	}
	test->val1 = val1;
	test->val2 = val2;
}

test_op_t
sttype_test_get_op(stnode_t *node)
{
	ws_assert_magic(node, TEST_MAGIC);
	return ((test_t *)node)->op;
}

const char *
sttype_test_todisplay(test_op_t op)
{
	const char *s;

	switch(op) {
		case TEST_OP_EXISTS:
			s = "exists";
			break;
		case TEST_OP_NOT:
			s = "!";
			break;
		case TEST_OP_AND:
			s = "&&";
			break;
		case TEST_OP_OR:
			s = "||";
			break;
		case TEST_OP_ANY_EQ:
			s = "==";
			break;
		case TEST_OP_ALL_NE:
			s = "!=";
			break;
		case TEST_OP_ANY_NE:
			s = "~=";
			break;
		case TEST_OP_GT:
			s = ">";
			break;
		case TEST_OP_GE:
			s = ">=";
			break;
		case TEST_OP_LT:
			s = "<";
			break;
		case TEST_OP_LE:
			s = "<=";
			break;
		case TEST_OP_BITWISE_AND:
			s = "&";
			break;
		case TEST_OP_CONTAINS:
			s = "contains";
			break;
		case TEST_OP_MATCHES:
			s = "matches";
			break;
		case TEST_OP_IN:
			s = "in";
			break;
		case TEST_OP_UNINITIALIZED:
			s = "<uninitialized>";
			break;
		default:
			s = "<null>";
			break;
	}
	return s;
}

void
sttype_test_get(stnode_t *node, test_op_t *p_op, stnode_t **p_val1, stnode_t **p_val2)
{
	test_t *test = stnode_data(node);
	ws_assert_magic(test, TEST_MAGIC);

	if (p_op)
		*p_op = test->op;
	if (p_val1)
		*p_val1 = test->val1;
	if (p_val2)
		*p_val2 = test->val2;
}

void
sttype_register_test(void)
{
	static sttype_t test_type = {
		STTYPE_TEST,
		"TEST",
		test_new,
		test_free,
		test_dup,
		test_tostr
	};

	sttype_register(&test_type);
}

/*
 * Editor modelines  -  https://www.wireshark.org/tools/modelines.html
 *
 * Local variables:
 * c-basic-offset: 8
 * tab-width: 8
 * indent-tabs-mode: t
 * End:
 *
 * vi: set shiftwidth=8 tabstop=8 noexpandtab:
 * :indentSize=8:tabSize=8:noTabs=false:
 */
