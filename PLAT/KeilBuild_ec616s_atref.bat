@echo off
@echo %PATH% | findstr /c:"%~dp0tools/msys64/usr/bin">nul
@if %errorlevel% equ 1 set PATH=%~dp0tools/msys64/usr/bin;%PATH%
rem @set PATH=%~dp0tools/msys64/usr/bin;%PATH%

set PROJECT_NAME=at_command
set BOARD_NAME=ec616s_0h00
set CHIP_NAME=ec616s

set KEILCC_PATH="C:/Keil_v5/ARM/ARMCC/bin/"
set PROTOCOL_TOOL_DB_PATH="..\PROTOCOL\Tool\UnilogPsParser\DB"
set COMMON_TOOL_DB_PATH="..\TOOLS\UniLogParser\DB"

echo build.bat version 20180330
echo KEILCC_PATH: %KEILCC_PATH%

if not %KEILCC_PATH% == "" (
   if not exist %KEILCC_PATH% (
      echo ERROR: Please check KEILCC_PATH setting, exit!!!
      goto end
   )
) else (
	echo ERROR: Please set KEILCC_PATH firstly, exit!!!
	goto end
)


set PARAMETERS=%1
if xx%PARAMETERS%==xx  (
echo no input paramter, use default build setting
echo default Board   is: %BOARD_NAME%
echo default Chip    is: %CHIP_NAME%
echo default Project is: %PROJECT_NAME%
)
echo ec616s with at ref!!!
rem --------- board/project parsing begain------------------

echo BOARD_NAME: %PARAMETERS% | findstr "ec616_0h00"
if not errorlevel 1 (
  set BOARD_NAME=ec616_0h00
  set CHIP_NAME=ec616
)

echo BOARD_NAME: %PARAMETERS% | findstr "ec616s_0h00"
if not errorlevel 1 (
  set BOARD_NAME=ec616s_0h00
  set CHIP_NAME=ec616s
)

rem internal project start
echo PROJECT_NAME: %PARAMETERS% | findstr "sleep_example"
if not errorlevel 1 (
  set PROJECT_NAME=sleep_example
)


echo PROJECT_NAME: %PARAMETERS% | findstr "fs_example"
if not errorlevel 1 (
  set PROJECT_NAME=fs_example
)

echo PROJECT_NAME: %PARAMETERS% | findstr "lpuart_test"
if not errorlevel 1 (
  set PROJECT_NAME=lpuart_test
)

echo PROJECT_NAME: %PARAMETERS% | findstr "sensor_smoke_example"
if not errorlevel 1 (
  set PROJECT_NAME=sensor_smoke_example
)
rem internal project end

echo PROJECT_NAME: %PARAMETERS% | findstr "slpman_example"
if not errorlevel 1 (
  set PROJECT_NAME=slpman_example
)

echo PROJECT_NAME: %PARAMETERS% | findstr "at_command"
if not errorlevel 1 (
  set PROJECT_NAME=at_command
)


echo PROJECT_NAME: %PARAMETERS% | findstr "bootloader"
if not errorlevel 1 (
  set PROJECT_NAME=bootloader
)

echo PROJECT_NAME: %PARAMETERS% | findstr "driver_example"
if not errorlevel 1 (
  set PROJECT_NAME=driver_example
)

echo PROJECT_NAME: %PARAMETERS% | findstr "onenet_example"
if not errorlevel 1 (
  set PROJECT_NAME=onenet_example
)


echo PROJECT_NAME: %PARAMETERS% | findstr "socket_example"
if not errorlevel 1 (
  set PROJECT_NAME=socket_example
)

echo PROJECT_NAME: %PARAMETERS% | findstr "ec_socket_example"
if not errorlevel 1 (
  set PROJECT_NAME=ec_socket_example
)

echo PROJECT_NAME: %PARAMETERS% | findstr "https_example"
if not errorlevel 1 (
  set PROJECT_NAME=https_example
)

echo PROJECT_NAME: %PARAMETERS% | findstr "lwm2m_example"
if not errorlevel 1 (
  set PROJECT_NAME=lwm2m_example
)

echo PROJECT_NAME: %PARAMETERS% | findstr "pslibapi_example"
if not errorlevel 1 (
  set PROJECT_NAME=pslibapi_example
)

echo PROJECT_NAME: %PARAMETERS% | findstr "coap_example"
if not errorlevel 1 (
  set PROJECT_NAME=coap_example
)

echo PROJECT_NAME: %PARAMETERS% | findstr "mqtt_example"
if not errorlevel 1 (
  set PROJECT_NAME=mqtt_example
)



echo PROJECT_NAME: %PARAMETERS% | findstr "ct_example"
if not errorlevel 1 (
  set PROJECT_NAME=ct_example
)


echo PROJECT_NAME: %PARAMETERS% | findstr "multi_at_command"
if not errorlevel 1 (
  set PROJECT_NAME=multi_at_command
)
rem --------- if more board/project add here-------------
rem --------- board/project parsing end------------------


for /f "tokens=1* delims=" %%a in ('type ".\device\eigencomm\board\%BOARD_NAME%\%BOARD_NAME%.mk"') do (
	if "%%a" equ "BUILD_FOR_KEY_CUSTOMER1    = y" (
		echo BUILD_FOR_KEY_CUSTOMER1    = n
	) else (
		echo %%a
	)

)>>temp.txt

type temp.txt | findstr /v ECHO >>temp1.txt
del temp.txt
move temp1.txt ".\device\eigencomm\board\%BOARD_NAME%\%BOARD_NAME%.mk" >nul

rem default is enable AT REF
for /f "tokens=1* delims=" %%a in ('type ".\device\eigencomm\board\%BOARD_NAME%\%BOARD_NAME%.mk"') do (
	if "%%a" equ "PS_AT_REF=n" (
		echo PS_AT_REF=y
	) else if "%%a" equ "BUILD_AT_REF=n" (
		echo BUILD_AT_REF=y
	) else (
		echo %%a
	)

)>>temp.txt

type temp.txt | findstr /v ECHO >>temp1.txt
del temp.txt
move temp1.txt ".\device\eigencomm\board\%BOARD_NAME%\%BOARD_NAME%.mk" >nul

echo CUSTOMER_NAME: %PARAMETERS% | findstr "qcom"
if not errorlevel 1 (
for /f "tokens=1* delims=" %%a in ('type ".\device\eigencomm\board\%BOARD_NAME%\%BOARD_NAME%.mk"') do (
	if "%%a" equ "BUILD_FOR_KEY_CUSTOMER1    = n" (
		echo BUILD_FOR_KEY_CUSTOMER1    = y
	) else (
		echo %%a
	)

)>>temp.txt

type temp.txt | findstr /v ECHO >>temp1.txt
del temp.txt
move temp1.txt ".\device\eigencomm\board\%BOARD_NAME%\%BOARD_NAME%.mk" >nul
)



echo OPTION: %PARAMETERS% | findstr "help" 
if not errorlevel 1 (
echo "=========================================================================================="
echo "                                                                                          "                                                                                          
echo "---------------------------Show How to Build Project--------------------------------------"
echo "                                                                                          "                                                             
echo "==========================================================================================”
echo "                                                                                          "
echo "                                                                                          "    
echo "                         Keilbuild.bat Board-Project-Option                               "
echo "Board,Project,Option could be omitted, if so default Board and project will be used      "
echo "                                                                                          "
echo "                                                                                          "   
echo "******************************************************************************************"
echo "Supported Options:"
echo "      doc----run doxygen to generate documents                                            "
echo "      unilog----pre-parse the log before compile                                          "
echo "      clean----clean the build output files                                               "
echo "      clall----clean all output files for every board and project                         "
echo "      jekins_release----trigger jekins_release                                            "
echo "      allprojects----trigger compile all example projects under default board             "
echo "      list----list current supported boards and supported project for each board          "
echo "      help---show how to use the build script                                             "
echo "                                                                                          "
echo "                                                                                          " 
echo "       Example start -- Example start -- Example start -- Example start                   "
echo "------------------------------------------------------------------------------------------"
echo "       Keilbuild.bat  %BOARD_NAME%-driver_example"
echo "       build driver_example project for board %BOARD_NAME%"
echo "******************************************************************************************"
echo "       Keilbuild.bat  %BOARD_NAME%-bootloader"
echo "       build bootloader project for board %BOARD_NAME%"
echo "******************************************************************************************"
echo "       Keilbuild.bat  %BOARD_NAME%-driver_example-clean"
echo "       clean driver_example project for board %BOARD_NAME%"
echo "*******************************************************************************************"
echo "       Keilbuild.bat clean"
echo "       clean default project for default board
echo "*******************************************************************************************"
echo "       Keilbuild.bat clall"
echo "       clean all output files for every board and project
echo "*******************************************************************************************"
echo "       Keilbuild.bat unilog"
echo "       perform log preparse of default project for default board
echo "*******************************************************************************************"
echo "       Keilbuild.bat list"
echo "       list current supported boards and supported project for each board
echo "-------------------------------------------------------------------------------------------"
echo "       Example end -- Example end -- Example end  --  Example end                          "
goto end
)



echo OPTION: %PARAMETERS% | findstr "list" 
if not errorlevel 1 (
   echo ---------------Supported Board------------------------------------
   for /d %%i in ("project\*") do (echo %%~nxi )
   
   echo ------------------------------------------------------------------
   
   setlocal enabledelayedexpansion
   for /d %%i in ("project\*") do (
   set CURRENT_BOARD=%%~nxi
   echo *********supported projects for board: !CURRENT_BOARD!*************
   for /d %%i in ("project\!CURRENT_BOARD!\apps\*") do (echo ----%%~nxi )
   )
   endlocal
   
goto end
)



echo OPTION: %PARAMETERS% | findstr "doc" 
if not errorlevel 1 (
	pushd doxygen
	CALL genDoc.bat %CHIP_NAME%
	popd
	goto end
)

setlocal enabledelayedexpansion
echo OPTION: %PARAMETERS% | findstr "unilog"
if not errorlevel 1 (
	del /q  .\tools\debug_log.h
	if exist .\tools\comdblib.txt (
		echo start logprepass
		.\tools\LogPrePass.exe ..\ .\tools\debug_log.h .\tools\comdb.txt .\tools\para_%CHIP_NAME%.json .\tools\comdblib.txt
	) else (
			echo start logprepass b2

	  .\tools\LogPrePass.exe ..\ .\tools\debug_log.h .\tools\comdb.txt .\tools\para_%CHIP_NAME%.json
	)
	if not errorlevel 0 (
		echo fail
		echo     #######################################################################
		echo     ##                                                                   ##
		echo     ##                    ########    ###     ####  ##                   ##
		echo     ##                    ##         ## ##     ##   ##                   ##
		echo     ##                    ##        ##   ##    ##   ##                   ##
		echo     ##                    ######   ##     ##   ##   ##                   ##
		echo     ##                    ##       #########   ##   ##                   ##
		echo     ##                    ##       ##     ##   ##   ##                   ##
		echo     ##                    ##       ##     ##  ####  ########             ##
		echo     ##                                                                   ##
		echo     #######################################################################  
		goto end
	)	
	rem this header file will compile with device code
	copy .\tools\debug_log.h .\middleware\eigencomm\debug\inc\ || (goto:failHandle)
	
	if exist %PROTOCOL_TOOL_DB_PATH% (
		copy .\tools\comdb.txt %PROTOCOL_TOOL_DB_PATH% || (goto:failHandle)
		
	)
	if exist %COMMON_TOOL_DB_PATH% (
		copy .\tools\comdb.txt %COMMON_TOOL_DB_PATH% || (goto:failHandle)
	)
  
)
endlocal



echo OPTION: %PARAMETERS% | findstr "clall"
if not errorlevel 1 (
	make.exe -j4  clean-keilall TARGET=%BOARD_NAME% CROSS_COMPILE=%KEILCC_PATH% PROJECT=%PROJECT_NAME% 
	echo clean all done ok...
	goto end
)

echo OPTION: %PARAMETERS% | findstr "clean"
if not errorlevel 1 (
	make.exe -j4  clean TARGET=%BOARD_NAME% CROSS_COMPILE=%KEILCC_PATH% PROJECT=%PROJECT_NAME% 
	echo clean done ok...
	goto end
)



echo OPTION: %PARAMETERS% | findstr "verbose"
if not errorlevel 1 (
  set VERBOSE=1
) else (
  set VERBOSE=0
)


rem 供JENKINS编译使用,默认编译bootloader和AT_command工程
echo OPTION: %PARAMETERS% | findstr "jenkins_release"
if not errorlevel 1 (
  if exist .\out\%BOARD_NAME% (
		echo delete %BOARD_NAME% folder......
		rd .\out\%BOARD_NAME% /s /q
	)
	
	del /q  .\tools\debug_log.h

	if exist .\tools\comdblib.txt (
		.\tools\LogPrePass.exe ..\ .\tools\debug_log.h .\tools\comdb.txt .\tools\para_%CHIP_NAME%.json .\tools\comdblib.txt
	) else (
	  .\tools\LogPrePass.exe ..\ .\tools\debug_log.h .\tools\comdb.txt .\tools\para_%CHIP_NAME%.json
	)
	if not errorlevel 0 (
		echo fail
		echo     #######################################################################
		echo     ##                                                                   ##
		echo     ##                    ########    ###     ####  ##                   ##
		echo     ##                    ##         ## ##     ##   ##                   ##
		echo     ##                    ##        ##   ##    ##   ##                   ##
		echo     ##                    ######   ##     ##   ##   ##                   ##
		echo     ##                    ##       #########   ##   ##                   ##
		echo     ##                    ##       ##     ##   ##   ##                   ##
		echo     ##                    ##       ##     ##  ####  ########             ##
		echo     ##                                                                   ##
		echo     #######################################################################  
		goto end
	)	
	rem this header file will compile with device code
	copy .\tools\debug_log.h .\middleware\eigencomm\debug\inc\
	
	if exist %PROTOCOL_TOOL_DB_PATH% (
		copy .\tools\comdb.txt %PROTOCOL_TOOL_DB_PATH%
		
	)
	if exist %COMMON_TOOL_DB_PATH% (
		copy .\tools\comdb.txt %COMMON_TOOL_DB_PATH%
	)
	
  setlocal enabledelayedexpansion
  for /d %%i in ("project\*") do (
  	set CURRENT_BOARD=%%~nxi
    echo *********supported projects for board: !CURRENT_BOARD!*************
    for /d %%i in ("project\!CURRENT_BOARD!\apps\*") do (
    	set CURRENT_PROJECT=%%~nxi
    	echo ----!CURRENT_PROJECT!
    	echo > .failed.tmp
			(make.exe -j4 keilall TARGET=%BOARD_NAME% V=%VERBOSE% CROSS_COMPILE=%KEILCC_PATH% PROJECT=%%~nxi 2>&1 && del .failed.tmp) | tee.exe .\out\%TARGET%\%%~nxi_outbuildlog.txt
			
			copy .\out\%BOARD_NAME%\%%~nxi\*.axf .\out\%BOARD_NAME%\
			copy .\out\%BOARD_NAME%\%%~nxi\*.bin .\out\%BOARD_NAME%\
			copy .\out\%BOARD_NAME%\%%~nxi\*.map .\out\%BOARD_NAME%\
			
			if exist .failed.tmp (
			del .failed.tmp
			echo fail
			echo     #######################################################################
			echo     ##                                                                   ##
			echo     ##                    ########    ###     ####  ##                   ##
			echo     ##                    ##         ## ##     ##   ##                   ##
			echo     ##                    ##        ##   ##    ##   ##                   ##
			echo     ##                    ######   ##     ##   ##   ##                   ##
			echo     ##                    ##       #########   ##   ##                   ##
			echo     ##                    ##       ##     ##   ##   ##                   ##
			echo     ##                    ##       ##     ##  ####  ########             ##
			echo     ##                                                                   ##
			echo     #######################################################################  
			goto end
			)
			rem copy log database to output dir after compile successfully
			copy .\tools\comdb.txt .\out\%BOARD_NAME%\%CURRENT_PROJECT%\
    )
  )
  endlocal
  
	echo     #######################################################################
	echo     ##                                                                   ##
	echo     ##                 ########     ###     ######   ######              ##
	echo     ##                 ##     ##   ## ##   ##    ## ##    ##             ##
	echo     ##                 ##     ##  ##   ##  ##       ##                   ##
	echo     ##                 ########  ##     ##  ######   ######              ##
	echo     ##                 ##        #########       ##       ##             ##
	echo     ##                 ##        ##     ## ##    ## ##    ##             ##
	echo     ##                 ##        ##     ##  ######   ######              ##
	echo     ##                                                                   ##
	echo     #######################################################################
	
	echo build successfully
	goto end
)

rem 编译所有的project
echo OPTION: %PARAMETERS% | findstr "allprojects"
if not errorlevel 1 (
  if exist .\out\%BOARD_NAME% (
		echo delete %BOARD_NAME% folder......
		rd .\out\%BOARD_NAME% /s /q
	)
	
	del /q  .\tools\debug_log.h

	if exist .\tools\comdblib.txt (
		.\tools\LogPrePass.exe ..\ .\tools\debug_log.h .\tools\comdb.txt .\tools\para_%CHIP_NAME%.json .\tools\comdblib.txt
	) else (
	  .\tools\LogPrePass.exe ..\ .\tools\debug_log.h .\tools\comdb.txt .\tools\para_%CHIP_NAME%.json
	)
	if not errorlevel 0 (
		echo fail
		echo     #######################################################################
		echo     ##                                                                   ##
		echo     ##                    ########    ###     ####  ##                   ##
		echo     ##                    ##         ## ##     ##   ##                   ##
		echo     ##                    ##        ##   ##    ##   ##                   ##
		echo     ##                    ######   ##     ##   ##   ##                   ##
		echo     ##                    ##       #########   ##   ##                   ##
		echo     ##                    ##       ##     ##   ##   ##                   ##
		echo     ##                    ##       ##     ##  ####  ########             ##
		echo     ##                                                                   ##
		echo     #######################################################################  
		goto end
	)	
	rem this header file will compile with device code
	copy .\tools\debug_log.h .\middleware\eigencomm\debug\inc\ || (goto:failHandle)
	
	if exist %PROTOCOL_TOOL_DB_PATH% (
		copy .\tools\comdb.txt %PROTOCOL_TOOL_DB_PATH% || (goto:failHandle)
		
	)
	if exist %COMMON_TOOL_DB_PATH% (
		copy .\tools\comdb.txt %COMMON_TOOL_DB_PATH% || (goto:failHandle)
	)
	
  setlocal enabledelayedexpansion

    echo *********supported projects for board: %BOARD_NAME%*************
    for /d %%i in ("project\%BOARD_NAME%\apps\*") do (
    	set CURRENT_PROJECT=%%~nxi
    	echo ----%CURRENT_PROJECT%
    	echo > .failed.tmp
			(make.exe -j4 keilall TARGET=%BOARD_NAME% V=%VERBOSE% CROSS_COMPILE=%KEILCC_PATH% PROJECT=%%~nxi 2>&1 && del .failed.tmp) | tee.exe .\out\%TARGET%\%%~nxi_outbuildlog.txt
			
			if exist .failed.tmp (
			del .failed.tmp
			echo fail
			echo     #######################################################################
			echo     ##                                                                   ##
			echo     ##                    ########    ###     ####  ##                   ##
			echo     ##                    ##         ## ##     ##   ##                   ##
			echo     ##                    ##        ##   ##    ##   ##                   ##
			echo     ##                    ######   ##     ##   ##   ##                   ##
			echo     ##                    ##       #########   ##   ##                   ##
			echo     ##                    ##       ##     ##   ##   ##                   ##
			echo     ##                    ##       ##     ##  ####  ########             ##
			echo     ##                                                                   ##
			echo     #######################################################################  
			goto end
			)
			rem copy log database to output dir after compile successfully
			copy .\tools\comdb.txt .\out\%BOARD_NAME%\%CURRENT_PROJECT%\ || (goto:failHandle)
    )
  
  endlocal
  
	echo     #######################################################################
	echo     ##                                                                   ##
	echo     ##                 ########     ###     ######   ######              ##
	echo     ##                 ##     ##   ## ##   ##    ## ##    ##             ##
	echo     ##                 ##     ##  ##   ##  ##       ##                   ##
	echo     ##                 ########  ##     ##  ######   ######              ##
	echo     ##                 ##        #########       ##       ##             ##
	echo     ##                 ##        ##     ## ##    ## ##    ##             ##
	echo     ##                 ##        ##     ##  ######   ######              ##
	echo     ##                                                                   ##
	echo     #######################################################################
	
	echo build successfully
	goto end
)

set starttime=%time%
echo Start time: %date% %starttime%

echo > .failed.tmp
(make.exe -j4 keilall TARGET=%BOARD_NAME% V=%VERBOSE% CROSS_COMPILE=%KEILCC_PATH% PROJECT=%PROJECT_NAME% 2>&1 && del .failed.tmp) | tee.exe .\out\%TARGET%\outbuildlog.txt

if exist .failed.tmp (
del .failed.tmp
echo fail
echo     #######################################################################
echo     ##                                                                   ##
echo     ##                    ########    ###     ####  ##                   ##
echo     ##                    ##         ## ##     ##   ##                   ##
echo     ##                    ##        ##   ##    ##   ##                   ##
echo     ##                    ######   ##     ##   ##   ##                   ##
echo     ##                    ##       #########   ##   ##                   ##
echo     ##                    ##       ##     ##   ##   ##                   ##
echo     ##                    ##       ##     ##  ####  ########             ##
echo     ##                                                                   ##
echo     #######################################################################  
goto end
)

rem copy log database to output dir after compile successfully
cp .\tools\comdb.txt .\out\%BOARD_NAME%\%PROJECT_NAME%\ || (goto:failHandle)


:complete

set endtime=%time%
echo .
echo End time: %date% %endtime%

set /a h1=%starttime:~0,2%
set /a m1=1%starttime:~3,2%-100
set /a s1=1%starttime:~6,2%-100
set /a h2=%endtime:~0,2%
set /a m2=1%endtime:~3,2%-100
set /a s2=1%endtime:~6,2%-100
if %h2% LSS %h1% set /a h2=%h2%+24
set /a ts1=%h1%*3600+%m1%*60+%s1%
set /a ts2=%h2%*3600+%m2%*60+%s2%
set /a ts=%ts2%-%ts1%
set /a h=%ts%/3600
set /a m=(%ts%-%h%*3600)/60
set /a s=%ts%%%60
echo Built took %h% hours %m% minutes %s% seconds

echo     #######################################################################
echo     ##                                                                   ##
echo     ##                 ########     ###     ######   ######              ##
echo     ##                 ##     ##   ## ##   ##    ## ##    ##             ##
echo     ##                 ##     ##  ##   ##  ##       ##                   ##
echo     ##                 ########  ##     ##  ######   ######              ##
echo     ##                 ##        #########       ##       ##             ##
echo     ##                 ##        ##     ## ##    ## ##    ##             ##
echo     ##                 ##        ##     ##  ######   ######              ##
echo     ##                                                                   ##
echo     #######################################################################

echo build successfully

:end
goto:eof

:failHandle
echo fail
echo     #######################################################################
echo     ##                                                                   ##
echo     ##                    ########    ###     ####  ##                   ##
echo     ##                    ##         ## ##     ##   ##                   ##
echo     ##                    ##        ##   ##    ##   ##                   ##
echo     ##                    ######   ##     ##   ##   ##                   ##
echo     ##                    ##       #########   ##   ##                   ##
echo     ##                    ##       ##     ##   ##   ##                   ##
echo     ##                    ##       ##     ##  ####  ########             ##
echo     ##                                                                   ##
echo     ####################################################################### 
goto:eof

