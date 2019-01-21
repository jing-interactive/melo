cd %~dp0\vc2015
msbuild %proj% /p:Configuration=Debug /p:Platform=x64 /v:minimal /m
msbuild %proj% /p:Configuration=Release /p:Platform=x64 /v:minimal /m
REM msbuild %proj% /p:Configuration=Debug_Shared /p:Platform=x64 /v:minimal /m
REM msbuild %proj% /p:Configuration=Release_Shared /p:Platform=x64 /v:minimal /m
