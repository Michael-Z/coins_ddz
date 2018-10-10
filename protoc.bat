
cd proto
for %%i in (*.proto) do protoc.exe --cpp_out=./ %%i

pause