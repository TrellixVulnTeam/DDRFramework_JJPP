set PrePath=%CD%
echo The current directory is %~dp0
set CurrentPath=%~dp0
cd %CurrentPath%


xcopy /Y %CurrentPath%\..\..\DDRLocalServer\x64\Release\Config %CurrentPath%\..\..\www\bin\x64\Release\Config /s /i
xcopy /Y %CurrentPath%\..\..\DDRStreamRelayService\x64\Release\Config %CurrentPath%\..\..\www\bin\x64\Release\Config /s /i


xcopy /Y %CurrentPath%\..\..\DDRLocalServer\x64\Debug\Config %CurrentPath%\..\..\www\bin\x64\Debug\Config /s /i
xcopy /Y %CurrentPath%\..\..\DDRStreamRelayService\x64\Debug\Config %CurrentPath%\..\..\www\bin\x64\Debug\Config /s /i

cd %PrePath%