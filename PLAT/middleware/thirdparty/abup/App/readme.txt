
20200525 修改
1、针对开启了DTLS时，重新调试联网这块；
2、优化局部变量，尽量改为动态内存。


20200312
1.修改abup_wosun.lib为移芯EC616_移动版本移植包，回掉方式改回原来的adups_bl_main.c-->abup_patch_progress_ext，在该函数内部调用
AbupUpgradeProgressCallback进行打印pre patch和nomral patch的进度。
2.修改打印进度支持选择UART1接口上报方式打印。

20200309
1.在bootloader中增加“升级进度回调函数” ：adups_bl_main.c --> AbupUpgradeProgressCallback
2.abup_config.c 添加 abup_FOTA_PROJECT_INFO 结构体，用来设置fota项目信息；
3.优化fota流程。

20200213
1.单独申请一个task用来处理fota的timer,task size为4K，
  初始化flash和网络超时处理改为给task发消息处理.

20200205
1.abup_start_op_timer_callback 和 abup_start_download 的调用方式不使用定时器，改为延时方式；
2.新增 abup_thread.c ，客户自己调用API接口启动fota，函数有注释说明 ；
3.优化 abup_task.c 的代码，如果不需要 abup_thread.c ，abup_task.c和 abup_task.h可以不修改;
4.#define ABUP_FOTA_TASK_STACKSIZE        	(1024 * 8) ；
5. ABUP_DOWNLOAD_MAX_RETRY 减少重试次数 .
6.库里的log是 ABUP_FOTA ，外围文件log是 ABUP_APP
7.此次修改增加了 abup_thread.c 里的log

20200116
1.增加 opencpu_fota_event_cb 回调结果;
2.boot 增加升级失败写入标志符。

20200114
1.加大组包的buff长度，300字节加大到512；如果超过512，return ;
2.修改abup_timer.c 文件编译警告。

20200111
一、文件更新：
1.abup_timer.c 此文件增加到以下路径：
\middleware\thirdparty\abup\App\Src

2.替换abup_app_private.lib文件到以下路径：
\prebuild\PLAT\lib\keil

二、修改点：
1.优化弱信号压力测试时连接失败Timer处理机制。
2.提取出Timer接口。
3.修改abup_lwm2mclient2()接口函数返回值，
  a.返回值为1，表示连接失败。
  b.返回值为0，表正连接正常。