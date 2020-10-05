#Commands for generating a fresh barebones drod5_0.dat file w/ MIDs imported
cd ..\DRODUtil\Release
DRODUtil.exe delete
DRODUtil.exe create
DRODUtil.exe import ..\..\Texts
