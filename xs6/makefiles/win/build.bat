nmake debug /c /f "xsr6.mak"
nmake debug /c /f "xsc6.mak"
nmake debug /c /f "xsl6.mak"
nmake debug /c /f "tools.mak"
nmake debug /c /f "xsbug.mak"

echo %%F_HOME%%\xs6\bin\win\debug\xsr6 -a %%F_HOME%%\xs6\bin\win\debug\modules\tools.xsa kprconfig %%* 1> %F_HOME%\xs6\bin\win\debug\kprconfig6.bat
echo %%F_HOME%%\xs6\bin\win\debug\xsr6 -a %%F_HOME%%\xs6\bin\win\debug\modules\tools.xsa kpr2js %%* 1> %F_HOME%\xs6\bin\win\debug\kpr2js6.bat
echo %%F_HOME%%\xs6\bin\win\debug\xsr6 -a %%F_HOME%%\xs6\bin\win\debug\modules\tools.xsa xs2js %%* 1> %F_HOME%\xs6\bin\win\debug\xs2js6.bat

nmake release /c /f "xsr6.mak"
nmake release /c /f "xsc6.mak"
nmake release /c /f "xsl6.mak"
nmake release /c /f "tools.mak"
nmake release /c /f "xsbug.mak"

echo %%F_HOME%%\xs6\bin\win\release\xsr6 -a %%F_HOME%%\xs6\bin\win\release\modules\tools.xsa kprconfig %%* 1> %F_HOME%\xs6\bin\win\release\kprconfig6.bat
echo %%F_HOME%%\xs6\bin\win\release\xsr6 -a %%F_HOME%%\xs6\bin\win\release\modules\tools.xsa kpr2js %%* 1> %F_HOME%\xs6\bin\win\release\kpr2js6.bat
echo %%F_HOME%%\xs6\bin\win\release\xsr6 -a %%F_HOME%%\xs6\bin\win\release\modules\tools.xsa xs2js %%* 1> %F_HOME%\xs6\bin\win\release\xs2js6.bat



