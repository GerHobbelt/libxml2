#ifndef __XML_MONOLITHIC_EX_H__
#define __XML_MONOLITHIC_EX_H__

#if defined(BUILD_MONOLITHIC)

#ifdef __cplusplus
extern "C" {
#endif

extern int xml_gio_bread_example_main(int argc, const char** argv);
extern int xml_nanohttp_main(int argc, const char** argv);
extern int xml_runsuite_tests_main(int argc, const char** argv);
extern int xml_runtest_main(int argc, const char** argv);
extern int xml_runxmlconfig_main(int argc, const char** argv);
extern int xml_schematron_main(int argc, const char** argv);
extern int xml_testapi_main(int argc, const char** argv);
extern int xml_testC14N_main(int argc, const char** argv);
extern int xml_testdict_main(void);
extern int xml_testhtml_main(int argc, const char** argv);
extern int xml_testlimits_main(int argc, const char** argv);
extern int xml_testmodule_main(int argc, const char** argv);
extern int xml_testreader_main(int argc, const char** argv);
extern int xml_testrecurse_main(int argc, const char** argv);
extern int xml_testregexp_main(int argc, const char** argv);
extern int xml_testrelax_main(int argc, const char** argv);
extern int xml_testSAX_main(int argc, const char** argv);
extern int xml_testschemas_main(int argc, const char** argv);
extern int xml_testURI_main(int argc, const char** argv);
extern int xml_testXPath_main(int argc, const char** argv);
extern int xml_xmlcatalog_main(int argc, const char** argv);
extern int xml_xmllint_main(int argc, const char** argv);
extern int xml_test_xmlreader_main(int argc, const char** argv);

extern int xml_gio_bread_example_main(int argc, const char** argv);
extern int xml_io1_example_main(void);
extern int xml_io2_example_main(void);
extern int xml_parse1_example_main(int argc, const char** argv);
extern int xml_parse2_example_main(int argc, const char** argv);
extern int xml_parse3_example_main(void);
extern int xml_parse4_example_main(int argc, const char** argv);
extern int xml_reader1_example_main(int argc, const char** argv);
extern int xml_reader2_example_main(int argc, const char** argv);
extern int xml_reader3_example_main(int argc, const char** argv);
extern int xml_reader4_example_main(int argc, const char** argv);
extern int xml_testWriter_example_main(void);
extern int xml_tree1_example_main(int argc, const char** argv);
extern int xml_tree2_example_main(int argc, const char** argv);
extern int xml_xpath1_example_main(int argc, const char** argv);
extern int xml_xpath2_example_main(int argc, const char** argv);

extern int xml_testchar_main(void);
extern int xml_testparser_main(void);

#ifdef __cplusplus
}
#endif

#endif

#endif /* __XML_ERROR_H__ */
