@echo off
if %errorlevel%==0 echo "errorlevel check works"
if "%~1"=="" echo "no args"
if not defined FOO echo "not defined works"
echo Done