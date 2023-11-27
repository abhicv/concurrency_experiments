@echo off
if not exist bin mkdir bin
pushd bin
cl /nologo /Zi ../source/matrix_multiply_demo.c
cl /nologo /Zi ../source/data_race_demo.c
cl /nologo /Zi ../source/httpclient.c /link Winhttp.lib
popd bin