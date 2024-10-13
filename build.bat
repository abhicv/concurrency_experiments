@echo off
if not exist bin mkdir bin
pushd bin
cl /nologo /Zi ../source/matrix_multiply_demo.c
@REM cl /nologo /Zi ../source/data_race_demo.c
@REM cl /nologo /Zi ../source/httpclient.c /link Winhttp.lib
@REM httpclient.exe
@REM data_race_demo.exe
matrix_multiply_demo.exe
popd bin