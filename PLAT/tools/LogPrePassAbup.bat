set ABUPLIB_PATH=.\middleware\thirdparty\abup\App\
rem set ABUPLIB_PATH=.\middleware\thirdparty\abup\App\core

if not %ABUPLIB_PATH% == "" (
   if exist %ABUPLIB_PATH% (
      echo : LogPrePass %ABUPLIB_PATH% !!!
      cp  .\middleware\eigencomm\debug\inc\Debug_trace.h %ABUPLIB_PATH%\Debug_trace.h
      .\tools\LogPrePass.exe %ABUPLIB_PATH% .\tools\debug_log_abup.h .\tools\comdb_abup.txt .\tools\para.json
      del /q  %ABUPLIB_PATH%\Debug_trace.h
      
   )
) 


