cd ../asset/shader
if not exist "spv" mkdir "spv"

for /r "glsl" %%i in (*) do (
echo %%i
glslangValidator.exe %%i -V -o ./spv/%%~ni.spv
)

pause