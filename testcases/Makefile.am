AM_CFLAGS = -Wall

noinst_PROGRAMS = test-osxcart
test_osxcart_CPPFLAGS = -I$(top_srcdir) -DTESTFILEDIR=\""$(abs_srcdir)"\"
test_osxcart_SOURCES = test-plist.c test-rtf.c test.c
test_osxcart_CFLAGS = @TEST_CFLAGS@ $(AM_CFLAGS)
test_osxcart_LDADD = @TEST_LIBS@ ../osxcart/libosxcart.la

check-local: test-report

.PHONY: test test-report perf-report full-report
test: test-osxcart
	GNOME_DISABLE_CRASH_DIALOG=1 gtester test-osxcart
test-report: test-osxcart 
	GNOME_DISABLE_CRASH_DIALOG=1 gtester -k test-osxcart -o=testlog.xml; \
	gtester-report testlog.xml > testlog.html
perf-report: test-osxcart
	GNOME_DISABLE_CRASH_DIALOG=1 gtester -k -m=perf test-osxcart -o=testlog.xml; \
	gtester-report testlog.xml > testlog.html
full-report: test-osxcart
	GNOME_DISABLE_CRASH_DIALOG=1 gtester -k -m=perf -m=thorough test-osxcart -o=testlog.xml; \
	gtester-report testlog.xml > testlog.html

CLEANFILES = testlog.xml testlog.html

EXTRA_DIST = \
	Settings.plist \
	badlyformed.fail.plist \
	extraneous_key.fail.plist \
	extraneous_object.fail.plist \
	incomplete.fail.plist \
	manifest.plist \
	missing_key.fail.plist \
	missing_version.fail.plist \
	no_data.fail.plist \
	no_plist.fail.plist \
	oneofeach.plist \
	wrong_attribute.fail.plist \
	wrong_version.fail.plist \
	p004_hello_world.rtf \
	p005a_hello_world.rtf \
	p005b_hello_world.rtf \
	p005c_hello_world.rtf \
	p006a_hello_world.rtf \
	p006b_hello_world.rtf \
	p007_salvete_omnes.rtf \
	p014_annalum.rtf \
	p015_annalum.rtf \
	p018_annalum.rtf \
	p023_martial.rtf \
	p024_annalum.rtf \
	p025_annalum.rtf \
	p027_grep.rtf \
	p032a_scream.rtf \
	p032b_scream.rtf \
	p034_daodejing.rtf \
	p044_vote.rtf \
	p045_vote.rtf \
	p046_vote_with_prelim.rtf \
	p048_zimgir.rtf \
	p049a_nondualist.rtf \
	p049b_supermegacomment.rtf \
	p050_seti_loewe.rtf \
	p051a_mysterious.rtf \
	p051b_chaucer.rtf \
	p052a_chaucer.rtf \
	p053_columns.rtf \
	p055_columns.rtf \
	p056_footnotes.rtf \
	p057_yow.rtf \
	p058a_yow.rtf \
	p058b_link.rtf \
	p058c_link.rtf \
	p059_margins.rtf \
	p060_horizrule.rtf \
	p060_landscape_a4.rtf \
	p060_landscape_usletter.rtf \
	p060_twoup_a4.rtf \
	p060_twoup_us.rtf \
	p061_horizrule.rtf \
	p062_horizrule.rtf \
	p063a_image.rtf \
	p063b_image.rtf \
	p063c_image.rtf \
	p065_vhcenter.rtf \
	p069_styles.rtf \
	p076a_table_single_cell_no_border.rtf \
	p076b_table_2x1_no_border.rtf \
	p076c_table_2x2_no_border.rtf \
	p077_table_2x2_groups_no_border.rtf \
	p078_table_cell_stretches_down_still_no_border.rtf \
	p082_table_all_borders.rtf \
	p083_table_some_borders.rtf \
	p085a_table_v_alignment.rtf \
	p085b_table_h_alignment.rtf \
	p086a_table_all_alignments.rtf \
	p086b_table_all_alignments_with_borders.rtf \
	crazy_fonttable.rtf \
	outtake_crazy_fonttable.rtf \
	outtake_latin.rtf \
	outtake_simplicity.rtf \
	DefaultText.rtf \
	minimal.rtf \
	RtfInterpreterTest_0.rtf \
	RtfInterpreterTest_1.rtf \
	RtfInterpreterTest_2.rtf \
	RtfInterpreterTest_3.rtf \
	RtfInterpreterTest_4.rtf \
	RtfInterpreterTest_5.rtf \
	RtfInterpreterTest_6.rtf \
	RtfInterpreterTest_7.rtf \
	RtfInterpreterTest_8.rtf \
	RtfInterpreterTest_9.rtf \
	RtfInterpreterTest_10.rtf \
	RtfInterpreterTest_11.rtf \
	RtfInterpreterTest_12.rtf \
	RtfInterpreterTest_13.rtf \
	RtfInterpreterTest_14.rtf \
	RtfInterpreterTest_15.rtf \
	RtfInterpreterTest_16.rtf \
	RtfInterpreterTest_17.rtf \
	RtfInterpreterTest_18.rtf \
	RtfInterpreterTest_19.rtf \
	RtfInterpreterTest_20.rtf \
	RtfInterpreterTest_21.rtf \
	RtfInterpreterTest_22.rtf \
	RtfInterpreterTest_fail_0.rtf \
	RtfInterpreterTest_fail_1.rtf \
	RtfInterpreterTest_fail_2.rtf \
	RtfInterpreterTest_fail_3.rtf \
	RtfInterpreterTest_fail_4.rtf \
	RtfParserTest_0.rtf \
	RtfParserTest_1.rtf \
	RtfParserTest_2.rtf \
	RtfParserTest_3.rtf \
	RtfParserTest_fail_0.rtf \
	RtfParserTest_fail_1.rtf \
	RtfParserTest_fail_2.rtf \
	RtfParserTest_fail_3.rtf \
	RtfParserTest_fail_5.rtf \
	RtfParserTest_fail_6.rtf \
	rtfdtest.rtfd/TXT.rtf \
	rtfdtest.rtfd/rtfpocketguide.jpeg


-include $(top_srcdir)/git.mk
