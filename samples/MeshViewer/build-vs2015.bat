cd %~dp0\vc2015
set proj=MeloViewer.sln
set msbuild="C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\MSBuild\Current\Bin\amd64\MSBuild.exe"
REM msbuild %proj% /p:Configuration=Debug /p:Platform=x64 /v:minimal /m
%msbuild% %proj% /p:Configuration=Release /p:Platform=x64 /v:minimal /m
REM msbuild %proj% /p:Configuration=Debug_Shared /p:Platform=x64 /v:minimal /m
REM msbuild %proj% /p:Configuration=Release_Shared /p:Platform=x64 /v:minimal /m
cd ..