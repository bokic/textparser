@echo off

cls

for /f "delims=" %%i in ('git describe --tags --dirty') do set git_describe=%%i

mkdir textparser\include

copy ..\bin\ccat.exe textparser
copy ..\bin\cfml_validator.exe textparser
copy ..\bin\textparser.exe textparser

copy ..\bin\textparser.dll textparser
copy ..\bin\textparser-json.dll textparser

copy ..\bin\textparser.lib textparser
copy ..\bin\textparser-json.lib textparser

copy ..\include\textparser.h textparser\include
copy ..\include\textparser-json.h textparser\include

copy ..\bin\json-c.dll textparser
copy ..\bin\pcre2-8.dll textparser
copy ..\bin\pcre2-16.dll textparser
copy ..\bin\pcre2-32.dll textparser
copy ..\bin\pcre2-posix.dll textparser

tar -a -c -f "textparser-%git_describe%.zip" "textparser\ccat.exe" "textparser\cfml_validator.exe" "textparser\textparser.exe" "textparser\textparser.dll" "textparser\textparser-json.dll"  "textparser\textparser.lib" "textparser\textparser-json.lib" "textparser\include\textparser.h" "textparser\include\textparser-json.h" "textparser\json-c.dll" "textparser\pcre2-8.dll" "textparser\pcre2-16.dll" "textparser\pcre2-32.dll" "textparser\pcre2-posix.dll"

rmdir /s /q textparser
