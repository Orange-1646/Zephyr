cd ../asset/shader
if not exist "spv" mkdir "spv"

for /r "glsl" %%i in (*) do (
glslangValidator.exe %%i -V -o ./spv/%%~ni.spv
)

pause