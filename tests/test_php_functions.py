"""Signature generator regressions; no PHP installation required."""
import importlib.util
from pathlib import Path
import unittest

spec = importlib.util.spec_from_file_location('generator', Path(__file__).resolve().parents[1] / 'src/validation/generate_php_functions.py')
generator = importlib.util.module_from_spec(spec)
spec.loader.exec_module(generator)


class PHPFunctionsTests(unittest.TestCase):
    def test_macro_arguments(self):
        self.assertEqual(generator.arguments('0, name, WRAP(1, 2), "a, \\"b\\""'),
                         ['0', 'name', 'WRAP(1, 2)', '"a, \\"b\\""'])
        with self.assertRaises(ValueError):
            generator.arguments('0, "unfinished')

    def test_optional_variadic_aliases_and_platform_variants(self):
        source = '''
ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_base, 0, 1, IS_STRING, 0)
 ZEND_ARG_TYPE_INFO(0, format, IS_STRING, 0)
 ZEND_ARG_VARIADIC_INFO(0, values)
ZEND_END_ARG_INFO()
#define arginfo_alias arginfo_base
#define arginfo_alias2 arginfo_alias
ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_base, 0, 0, IS_STRING, 0)
ZEND_END_ARG_INFO()
static const zend_function_entry ext_functions[] = {
 ZEND_FE(sample, arginfo_alias2)
 ZEND_RAW_FENTRY(ZEND_NS_NAME("Demo", "Alias"), zif_sample, arginfo_alias, 0, NULL, NULL)
};
static const zend_function_entry class_Test_methods[] = {
 ZEND_ME(Test, method, missing, 0)
};
'''
        result = generator.signatures(source)
        self.assertEqual(set(result), {'sample', 'demo\\alias'})
        self.assertEqual(result['sample'], [('STRING', [('$format', True, False, 'STRING'),
                                                       ('$values', False, True, 'ANY')]), ('STRING', [])])
        self.assertEqual(result['sample'], result['demo\\alias'])

    def test_sapi_function_tables_cross_reference_arginfo(self):
        blocks, aliases, result = {}, {}, {}
        generator.signatures('''
ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_cli_get_process_title, 0, 0, IS_STRING, 1)
ZEND_END_ARG_INFO()
ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_getallheaders, 0, 0, IS_ARRAY, 0)
ZEND_END_ARG_INFO()
''', blocks, aliases, result)
        generator.signatures('''
const zend_function_entry server_additional_functions[] = {
 PHP_FE(cli_get_process_title, arginfo_cli_get_process_title)
 PHP_FALIAS(getallheaders, apache_request_headers, arginfo_getallheaders)
 PHP_FE_END
};
''', blocks, aliases, result)
        self.assertEqual(set(result), {'cli_get_process_title', 'getallheaders'})
        self.assertEqual(result['cli_get_process_title'], [('STRING', [])])
        self.assertEqual(result['getallheaders'], [('ARRAY', [])])

    def test_unsupported_function_entry_macro_fails(self):
        with self.assertRaisesRegex(ValueError, 'Unsupported function entry'):
            generator.signatures('''
static const zend_function_entry functions[] = {
 ZEND_ME(Test, method, missing, 0)
};''')

    def test_alias_cycle_fails(self):
        with self.assertRaisesRegex(ValueError, 'Alias cycle'):
            generator.signatures('''
#define arginfo_a arginfo_b
#define arginfo_b arginfo_a
static const zend_function_entry functions[] = {
 ZEND_FE(cycle, arginfo_a)
};''')

    def test_types(self):
        self.assertEqual(generator.attr('ZEND_ARG_OBJ_INFO', 'Example'), 'OBJECT')
        self.assertEqual(generator.attr('ZEND_ARG_TYPE_MASK', 'MAY_BE_LONG|MAY_BE_STRING'), 'ANY')
        self.assertEqual(generator.attr('ZEND_ARG_TYPE_INFO', '_IS_BOOL'), 'BOOLEAN')

    def test_invalid_required_count_fails(self):
        with self.assertRaisesRegex(ValueError, 'Required count'):
            generator.signatures('''
ZEND_BEGIN_ARG_INFO_EX(arginfo_bad, 0, 0, 2)
 ZEND_ARG_INFO(0, one)
ZEND_END_ARG_INFO()
''')


if __name__ == '__main__':
    unittest.main()
