
20200525 �޸�
1����Կ�����DTLSʱ�����µ���������飻
2���Ż��ֲ�������������Ϊ��̬�ڴ档


20200312
1.�޸�abup_wosun.libΪ��оEC616_�ƶ��汾��ֲ�����ص���ʽ�Ļ�ԭ����adups_bl_main.c-->abup_patch_progress_ext���ڸú����ڲ�����
AbupUpgradeProgressCallback���д�ӡpre patch��nomral patch�Ľ��ȡ�
2.�޸Ĵ�ӡ����֧��ѡ��UART1�ӿ��ϱ���ʽ��ӡ��

20200309
1.��bootloader�����ӡ��������Ȼص������� ��adups_bl_main.c --> AbupUpgradeProgressCallback
2.abup_config.c ��� abup_FOTA_PROJECT_INFO �ṹ�壬��������fota��Ŀ��Ϣ��
3.�Ż�fota���̡�

20200213
1.��������һ��task��������fota��timer,task sizeΪ4K��
  ��ʼ��flash�����糬ʱ�����Ϊ��task����Ϣ����.

20200205
1.abup_start_op_timer_callback �� abup_start_download �ĵ��÷�ʽ��ʹ�ö�ʱ������Ϊ��ʱ��ʽ��
2.���� abup_thread.c ���ͻ��Լ�����API�ӿ�����fota��������ע��˵�� ��
3.�Ż� abup_task.c �Ĵ��룬�������Ҫ abup_thread.c ��abup_task.c�� abup_task.h���Բ��޸�;
4.#define ABUP_FOTA_TASK_STACKSIZE        	(1024 * 8) ��
5. ABUP_DOWNLOAD_MAX_RETRY �������Դ��� .
6.�����log�� ABUP_FOTA ����Χ�ļ�log�� ABUP_APP
7.�˴��޸������� abup_thread.c ���log

20200116
1.���� opencpu_fota_event_cb �ص����;
2.boot ��������ʧ��д���־����

20200114
1.�Ӵ������buff���ȣ�300�ֽڼӴ�512���������512��return ;
2.�޸�abup_timer.c �ļ����뾯�档

20200111
һ���ļ����£�
1.abup_timer.c ���ļ����ӵ�����·����
\middleware\thirdparty\abup\App\Src

2.�滻abup_app_private.lib�ļ�������·����
\prebuild\PLAT\lib\keil

�����޸ĵ㣺
1.�Ż����ź�ѹ������ʱ����ʧ��Timer������ơ�
2.��ȡ��Timer�ӿڡ�
3.�޸�abup_lwm2mclient2()�ӿں�������ֵ��
  a.����ֵΪ1����ʾ����ʧ�ܡ�
  b.����ֵΪ0����������������