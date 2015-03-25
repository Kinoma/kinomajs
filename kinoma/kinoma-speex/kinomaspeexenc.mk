<?xml version="1.0" encoding="UTF-8"?>
<!--
|     Copyright (C) 2010-2015 Marvell International Ltd.
|     Copyright (C) 2002-2010 Kinoma, Inc.
|
|     Licensed under the Apache License, Version 2.0 (the "License");
|     you may not use this file except in compliance with the License.
|     You may obtain a copy of the License at
|
|      http://www.apache.org/licenses/LICENSE-2.0
|
|     Unless required by applicable law or agreed to in writing, software
|     distributed under the License is distributed on an "AS IS" BASIS,
|     WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
|     See the License for the specific language governing permissions and
|     limitations under the License.
-->
<makefile>

<input name="speex-1.2rc1/include/"/>
<input name="speex-1.2rc1/include/speex/"/>
<input name="speex-1.2rc1/libspeex/"/>

<platform name="Windows">
<input name="sources/win32/"/>
</platform>

<platform name="android,android-cmake">
<input name="sources/android/"/>
</platform>

<platform name="MacOSX,mac-cmake">
<input name="sources/android/"/>
</platform>

<platform name="iPhone">
<input name="sources/android/"/>
</platform>

<include name="/makefiles/xsFskDefaults.mk"/>

<wrap name="sources/kinomaspeexextensionenc.c"/>
<wrap name="sources/kinomaspeexenc.c"/>
<wrap name="sources/kinomaspeexutils.c"/>

<platform name="Windows">
<common>
C_OPTIONS =  /D "_WIN32_WINNT=0x0400" /D "HAVE_CONFIG_H" $(C_OPTIONS)
</common>
<wrap name="speex-1.2rc1/libspeex/kinoma_speex.c"/>
<wrap name="speex-1.2rc1/libspeex/bits.c"/>
<wrap name="speex-1.2rc1/libspeex/cb_search.c"/>
<wrap name="speex-1.2rc1/libspeex/exc_10_16_table.c"/>
<wrap name="speex-1.2rc1/libspeex/exc_10_32_table.c"/>
<wrap name="speex-1.2rc1/libspeex/exc_20_32_table.c"/>
<wrap name="speex-1.2rc1/libspeex/exc_5_256_table.c"/>
<wrap name="speex-1.2rc1/libspeex/exc_5_64_table.c"/>
<wrap name="speex-1.2rc1/libspeex/exc_8_128_table.c"/>
<wrap name="speex-1.2rc1/libspeex/filters.c"/>
<wrap name="speex-1.2rc1/libspeex/gain_table.c"/>
<wrap name="speex-1.2rc1/libspeex/gain_table_lbr.c"/>
<wrap name="speex-1.2rc1/libspeex/hexc_10_32_table.c"/>
<wrap name="speex-1.2rc1/libspeex/hexc_table.c"/>
<wrap name="speex-1.2rc1/libspeex/high_lsp_tables.c"/>
<wrap name="speex-1.2rc1/libspeex/lpc.c"/>
<wrap name="speex-1.2rc1/libspeex/lsp.c"/>
<wrap name="speex-1.2rc1/libspeex/lsp_tables_nb.c"/>
<wrap name="speex-1.2rc1/libspeex/ltp.c"/>
<wrap name="speex-1.2rc1/libspeex/modes.c"/>
<wrap name="speex-1.2rc1/libspeex/modes_wb.c"/>
<wrap name="speex-1.2rc1/libspeex/nb_celp.c"/>
<wrap name="speex-1.2rc1/libspeex/quant_lsp.c"/>
<wrap name="speex-1.2rc1/libspeex/sb_celp.c"/>
<wrap name="speex-1.2rc1/libspeex/speex.c"/>
<wrap name="speex-1.2rc1/libspeex/speex_callbacks.c"/>
<wrap name="speex-1.2rc1/libspeex/speex_header.c"/>
<wrap name="speex-1.2rc1/libspeex/vbr.c"/>
<wrap name="speex-1.2rc1/libspeex/vq.c"/>
<wrap name="speex-1.2rc1/libspeex/window.c"/>
</platform>

<platform name="MacOSX">
<common>
C_OPTIONS += -DOSX32=1 -DHAVE_CONFIG_H	            
</common>
<wrap name="speex-1.2rc1/libspeex/kinoma_speex.c"/>
<wrap name="speex-1.2rc1/libspeex/bits.c"/>
<wrap name="speex-1.2rc1/libspeex/cb_search.c"/>
<wrap name="speex-1.2rc1/libspeex/exc_10_16_table.c"/>
<wrap name="speex-1.2rc1/libspeex/exc_10_32_table.c"/>
<wrap name="speex-1.2rc1/libspeex/exc_20_32_table.c"/>
<wrap name="speex-1.2rc1/libspeex/exc_5_256_table.c"/>
<wrap name="speex-1.2rc1/libspeex/exc_5_64_table.c"/>
<wrap name="speex-1.2rc1/libspeex/exc_8_128_table.c"/>
<wrap name="speex-1.2rc1/libspeex/filters.c"/>
<wrap name="speex-1.2rc1/libspeex/gain_table.c"/>
<wrap name="speex-1.2rc1/libspeex/gain_table_lbr.c"/>
<wrap name="speex-1.2rc1/libspeex/hexc_10_32_table.c"/>
<wrap name="speex-1.2rc1/libspeex/hexc_table.c"/>
<wrap name="speex-1.2rc1/libspeex/high_lsp_tables.c"/>
<wrap name="speex-1.2rc1/libspeex/lpc.c"/>
<wrap name="speex-1.2rc1/libspeex/lsp.c"/>
<wrap name="speex-1.2rc1/libspeex/lsp_tables_nb.c"/>
<wrap name="speex-1.2rc1/libspeex/ltp.c"/>
<wrap name="speex-1.2rc1/libspeex/modes.c"/>
<wrap name="speex-1.2rc1/libspeex/modes_wb.c"/>
<wrap name="speex-1.2rc1/libspeex/nb_celp.c"/>
<wrap name="speex-1.2rc1/libspeex/quant_lsp.c"/>
<wrap name="speex-1.2rc1/libspeex/sb_celp.c"/>
<wrap name="speex-1.2rc1/libspeex/speex.c"/>
<wrap name="speex-1.2rc1/libspeex/speex_callbacks.c"/>
<wrap name="speex-1.2rc1/libspeex/speex_header.c"/>
<wrap name="speex-1.2rc1/libspeex/vbr.c"/>
<wrap name="speex-1.2rc1/libspeex/vq.c"/>
<wrap name="speex-1.2rc1/libspeex/window.c"/>
</platform>

<platform name="mac-cmake">
<common>
add_definitions(-DOSX32=1)
add_definitions(-DHAVE_CONFIG_H)
</common>
<wrap name="speex-1.2rc1/libspeex/kinoma_speex.c"/>
<wrap name="speex-1.2rc1/libspeex/bits.c"/>
<wrap name="speex-1.2rc1/libspeex/cb_search.c"/>
<wrap name="speex-1.2rc1/libspeex/exc_10_16_table.c"/>
<wrap name="speex-1.2rc1/libspeex/exc_10_32_table.c"/>
<wrap name="speex-1.2rc1/libspeex/exc_20_32_table.c"/>
<wrap name="speex-1.2rc1/libspeex/exc_5_256_table.c"/>
<wrap name="speex-1.2rc1/libspeex/exc_5_64_table.c"/>
<wrap name="speex-1.2rc1/libspeex/exc_8_128_table.c"/>
<wrap name="speex-1.2rc1/libspeex/filters.c"/>
<wrap name="speex-1.2rc1/libspeex/gain_table.c"/>
<wrap name="speex-1.2rc1/libspeex/gain_table_lbr.c"/>
<wrap name="speex-1.2rc1/libspeex/hexc_10_32_table.c"/>
<wrap name="speex-1.2rc1/libspeex/hexc_table.c"/>
<wrap name="speex-1.2rc1/libspeex/high_lsp_tables.c"/>
<wrap name="speex-1.2rc1/libspeex/lpc.c"/>
<wrap name="speex-1.2rc1/libspeex/lsp.c"/>
<wrap name="speex-1.2rc1/libspeex/lsp_tables_nb.c"/>
<wrap name="speex-1.2rc1/libspeex/ltp.c"/>
<wrap name="speex-1.2rc1/libspeex/modes.c"/>
<wrap name="speex-1.2rc1/libspeex/modes_wb.c"/>
<wrap name="speex-1.2rc1/libspeex/nb_celp.c"/>
<wrap name="speex-1.2rc1/libspeex/quant_lsp.c"/>
<wrap name="speex-1.2rc1/libspeex/sb_celp.c"/>
<wrap name="speex-1.2rc1/libspeex/speex.c"/>
<wrap name="speex-1.2rc1/libspeex/speex_callbacks.c"/>
<wrap name="speex-1.2rc1/libspeex/speex_header.c"/>
<wrap name="speex-1.2rc1/libspeex/vbr.c"/>
<wrap name="speex-1.2rc1/libspeex/vq.c"/>
<wrap name="speex-1.2rc1/libspeex/window.c"/>
</platform>

<platform name="iPhone">
<common>
C_OPTIONS += -DOSX32=1 -DHAVE_CONFIG_H	            
</common>
<wrap name="speex-1.2rc1/libspeex/kinoma_speex.c"/>
<wrap name="speex-1.2rc1/libspeex/bits.c"/>
<wrap name="speex-1.2rc1/libspeex/cb_search.c"/>
<wrap name="speex-1.2rc1/libspeex/exc_10_16_table.c"/>
<wrap name="speex-1.2rc1/libspeex/exc_10_32_table.c"/>
<wrap name="speex-1.2rc1/libspeex/exc_20_32_table.c"/>
<wrap name="speex-1.2rc1/libspeex/exc_5_256_table.c"/>
<wrap name="speex-1.2rc1/libspeex/exc_5_64_table.c"/>
<wrap name="speex-1.2rc1/libspeex/exc_8_128_table.c"/>
<wrap name="speex-1.2rc1/libspeex/filters.c"/>
<wrap name="speex-1.2rc1/libspeex/gain_table.c"/>
<wrap name="speex-1.2rc1/libspeex/gain_table_lbr.c"/>
<wrap name="speex-1.2rc1/libspeex/hexc_10_32_table.c"/>
<wrap name="speex-1.2rc1/libspeex/hexc_table.c"/>
<wrap name="speex-1.2rc1/libspeex/high_lsp_tables.c"/>
<wrap name="speex-1.2rc1/libspeex/lpc.c"/>
<wrap name="speex-1.2rc1/libspeex/lsp.c"/>
<wrap name="speex-1.2rc1/libspeex/lsp_tables_nb.c"/>
<wrap name="speex-1.2rc1/libspeex/ltp.c"/>
<wrap name="speex-1.2rc1/libspeex/modes.c"/>
<wrap name="speex-1.2rc1/libspeex/modes_wb.c"/>
<wrap name="speex-1.2rc1/libspeex/nb_celp.c"/>
<wrap name="speex-1.2rc1/libspeex/quant_lsp.c"/>
<wrap name="speex-1.2rc1/libspeex/sb_celp.c"/>
<wrap name="speex-1.2rc1/libspeex/speex.c"/>
<wrap name="speex-1.2rc1/libspeex/speex_callbacks.c"/>
<wrap name="speex-1.2rc1/libspeex/speex_header.c"/>
<wrap name="speex-1.2rc1/libspeex/vbr.c"/>
<wrap name="speex-1.2rc1/libspeex/vq.c"/>
<wrap name="speex-1.2rc1/libspeex/window.c"/>
</platform>

<platform name="android-cmake">
<common>
add_definitions(-DHAVE_CONFIG_H)
</common>
<wrap name="speex-1.2rc1/libspeex/kinoma_speex.c"/>
<wrap name="speex-1.2rc1/libspeex/bits.c"/>
<wrap name="speex-1.2rc1/libspeex/cb_search.c"/>
<wrap name="speex-1.2rc1/libspeex/exc_10_16_table.c"/>
<wrap name="speex-1.2rc1/libspeex/exc_10_32_table.c"/>
<wrap name="speex-1.2rc1/libspeex/exc_20_32_table.c"/>
<wrap name="speex-1.2rc1/libspeex/exc_5_256_table.c"/>
<wrap name="speex-1.2rc1/libspeex/exc_5_64_table.c"/>
<wrap name="speex-1.2rc1/libspeex/exc_8_128_table.c"/>
<wrap name="speex-1.2rc1/libspeex/filters.c"/>
<wrap name="speex-1.2rc1/libspeex/gain_table.c"/>
<wrap name="speex-1.2rc1/libspeex/gain_table_lbr.c"/>
<wrap name="speex-1.2rc1/libspeex/hexc_10_32_table.c"/>
<wrap name="speex-1.2rc1/libspeex/hexc_table.c"/>
<wrap name="speex-1.2rc1/libspeex/high_lsp_tables.c"/>
<wrap name="speex-1.2rc1/libspeex/lpc.c"/>
<wrap name="speex-1.2rc1/libspeex/lsp.c"/>
<wrap name="speex-1.2rc1/libspeex/lsp_tables_nb.c"/>
<wrap name="speex-1.2rc1/libspeex/ltp.c"/>
<wrap name="speex-1.2rc1/libspeex/modes.c"/>
<wrap name="speex-1.2rc1/libspeex/modes_wb.c"/>
<wrap name="speex-1.2rc1/libspeex/nb_celp.c"/>
<wrap name="speex-1.2rc1/libspeex/quant_lsp.c"/>
<wrap name="speex-1.2rc1/libspeex/sb_celp.c"/>
<wrap name="speex-1.2rc1/libspeex/speex.c"/>
<wrap name="speex-1.2rc1/libspeex/speex_callbacks.c"/>
<wrap name="speex-1.2rc1/libspeex/speex_header.c"/>
<wrap name="speex-1.2rc1/libspeex/vbr.c"/>
<wrap name="speex-1.2rc1/libspeex/vq.c"/>
<wrap name="speex-1.2rc1/libspeex/window.c"/>
</platform>

<platform name="android">
<common>
C_OPTIONS += -DHAVE_CONFIG_H
</common>
<wrap name="speex-1.2rc1/libspeex/kinoma_speex.c"/>
<wrap name="speex-1.2rc1/libspeex/bits.c"/>
<wrap name="speex-1.2rc1/libspeex/cb_search.c"/>
<wrap name="speex-1.2rc1/libspeex/exc_10_16_table.c"/>
<wrap name="speex-1.2rc1/libspeex/exc_10_32_table.c"/>
<wrap name="speex-1.2rc1/libspeex/exc_20_32_table.c"/>
<wrap name="speex-1.2rc1/libspeex/exc_5_256_table.c"/>
<wrap name="speex-1.2rc1/libspeex/exc_5_64_table.c"/>
<wrap name="speex-1.2rc1/libspeex/exc_8_128_table.c"/>
<wrap name="speex-1.2rc1/libspeex/filters.c"/>
<wrap name="speex-1.2rc1/libspeex/gain_table.c"/>
<wrap name="speex-1.2rc1/libspeex/gain_table_lbr.c"/>
<wrap name="speex-1.2rc1/libspeex/hexc_10_32_table.c"/>
<wrap name="speex-1.2rc1/libspeex/hexc_table.c"/>
<wrap name="speex-1.2rc1/libspeex/high_lsp_tables.c"/>
<wrap name="speex-1.2rc1/libspeex/lpc.c"/>
<wrap name="speex-1.2rc1/libspeex/lsp.c"/>
<wrap name="speex-1.2rc1/libspeex/lsp_tables_nb.c"/>
<wrap name="speex-1.2rc1/libspeex/ltp.c"/>
<wrap name="speex-1.2rc1/libspeex/modes.c"/>
<wrap name="speex-1.2rc1/libspeex/modes_wb.c"/>
<wrap name="speex-1.2rc1/libspeex/nb_celp.c"/>
<wrap name="speex-1.2rc1/libspeex/quant_lsp.c"/>
<wrap name="speex-1.2rc1/libspeex/sb_celp.c"/>
<wrap name="speex-1.2rc1/libspeex/speex.c"/>
<wrap name="speex-1.2rc1/libspeex/speex_callbacks.c"/>
<wrap name="speex-1.2rc1/libspeex/speex_header.c"/>
<wrap name="speex-1.2rc1/libspeex/vbr.c"/>
<wrap name="speex-1.2rc1/libspeex/vq.c"/>
<wrap name="speex-1.2rc1/libspeex/window.c"/>
</platform>

<include name="/makefiles/xsLibrary.mk"/>

</makefile>