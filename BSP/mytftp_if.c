#include "mytftp_if.h"
#include <string.h>
#include <stdio.h>
#include "main.h"
#include "flash_if.h"
#include "stdlib.h"
#include "string.h"

/* Define a base directory for TFTP access
 * ATTENTION: This code does NOT check for sandboxing,
 * i.e. '..' in paths is not checked! */
#ifndef LWIP_TFTP_EXAMPLE_BASE_DIR
#define LWIP_TFTP_EXAMPLE_BASE_DIR ""
#endif

/* Define this to a file to get via tftp client */
#ifndef LWIP_TFTP_EXAMPLE_CLIENT_FILENAME
#define LWIP_TFTP_EXAMPLE_CLIENT_FILENAME "LED.bin"
#endif

/* Define this to a server IP string */
#ifndef LWIP_TFTP_EXAMPLE_CLIENT_REMOTEIP
#define LWIP_TFTP_EXAMPLE_CLIENT_REMOTEIP "192.168.137.1"
#endif

__packed typedef struct 
{
  uint8_t   InitFlat;
  uint8_t   W_R_Mode;   //��дģʽ   1д  0��
  uint32_t  Flash_Addr ;
}Iap_FlashCont;


static  Iap_FlashCont   Iapflashcont ;

static void * OpenFile(const char* fname, const char* mode, u8_t write);
static void Close_File(void* handle);
static void Tftp_Error(void* handle, int err, const char* msg, int size);

static int Read_File(void* handle, void* buf, int bytes);
static int Write_File(void* handle, struct pbuf* p);

const struct tftp_context  tftpContext={       //TFTP SERVER/CLIENT �Խӽӿ�
 OpenFile,
 Close_File,
 Read_File,    
 Write_File,
 Tftp_Error, // For TFTP client only
};

static char full_filename[256];

static void *
tftp_open_file(const char* fname, u8_t is_write)
{
  snprintf(full_filename, sizeof(full_filename), "%s%s", LWIP_TFTP_EXAMPLE_BASE_DIR, fname);
  full_filename[sizeof(full_filename)-1] = 0;

  printf("���ļ�  %s \r\n",fname);    
  Iapflashcont.W_R_Mode = is_write ;
  Iapflashcont.W_R_Mode ? printf("д�ļ�\r\n") : printf("���ļ�\r\n");

  if (Iapflashcont.W_R_Mode == 1) {
	  
	FLASH_If_Init();   //����
    Iapflashcont.Flash_Addr = USER_FLASH_FIRST_PAGE_ADDRESS ;  //FLASH��ʼ��ַ
    printf("��ʼ����Flash  ʱ��ϳ� \r\n");  
	  
    if( FLASH_If_Erase(USER_FLASH_FIRST_PAGE_ADDRESS) == 0 )   //�����û���FLASH����
    {
      Iapflashcont.InitFlat =1 ;   //��ǳ�ʼ�����
      printf("Write File Init Succes \r\n");
    }
//    return (void*)fopen(full_filename, "wb");
	return (Iapflashcont.InitFlat) ? (&Iapflashcont) : NULL ;//�����ʼ���ɹ�  ������Ч���
  } else {
	  
    return (void*)fopen(full_filename, "rb");
  }
}

  /**
   *  ���ļ�  �����ļ����
   * @param const char* fname   �ļ���
   * @param const char* mode
   * @param u8_t write   ģʽ  1д  0��
   * @returns �ļ����
  */
static void * OpenFile(const char* fname, const char* mode, u8_t write)
{   
  printf("���ļ�  %s \r\n",fname);    
  printf("��ģʽ  %s \r\n",mode);
  Iapflashcont.W_R_Mode = write ;
  Iapflashcont.W_R_Mode ? printf("д�ļ�\r\n") : printf("���ļ�\r\n");
 
  if(Iapflashcont.W_R_Mode == 1)
  {
    FLASH_If_Init();   //����
    Iapflashcont.Flash_Addr = USER_FLASH_FIRST_PAGE_ADDRESS ;  //FLASH��ʼ��ַ
    printf("��ʼ����Flash  ʱ��ϳ� \r\n");  
    if( FLASH_If_Erase(USER_FLASH_FIRST_PAGE_ADDRESS) == 0 )   //�����û���FLASH����
    {
      Iapflashcont.InitFlat =1 ;   //��ǳ�ʼ�����
      printf("Write File Init Succes \r\n");
    }
  } //���Ϊ���ļ�ģʽ
  else if(memcmp(fname,"flash.bin",strlen("flash.bin"))==0)  //���Զ��ڲ�FLASH
  {
    Iapflashcont.InitFlat =1 ;   //��ǳ�ʼ�����
    printf("Read File Init Succes \r\n");
    Iapflashcont.Flash_Addr = FLASH_BASE ;  //FLASH��ʼ��ַ
  }
  return (Iapflashcont.InitFlat) ? (&Iapflashcont) : NULL ;  //�����ʼ���ɹ�  ������Ч���
}

  /**
   *  �ر��ļ����
   * @param None
   * @param None
   * @param None
   * @returns None
   */
static void Close_File(void* handle)
{
  Iap_FlashCont * Filehandle = (Iap_FlashCont *) handle ;
     
  FLASH_If_UnInit();  //FLASH����
  Filehandle->InitFlat = 0 ;
  printf("�ر��ļ�\r\n");  
  if(Filehandle->W_R_Mode)  //���֮ǰ��д�ļ�
  {
    typedef  void (*pFunction)(void);
    pFunction Jump_To_Application;
    uint32_t JumpAddress;
    /* Check if valid stack address (RAM address) then jump to user application */
   if (((*(__IO uint32_t*)USER_FLASH_FIRST_PAGE_ADDRESS) & 0x2FFE0000 ) == 0x20000000)
   {
	 printf("����������! \r\n");
     /* Jump to user application */
     JumpAddress = *(__IO uint32_t*) (USER_FLASH_FIRST_PAGE_ADDRESS + 4);
     Jump_To_Application =   (pFunction)JumpAddress;
     /* Initialize user application's Stack Pointer */
     __set_MSP(*(__IO uint32_t*) USER_FLASH_FIRST_PAGE_ADDRESS);
     Jump_To_Application();
     /* do nothing */
     while(1);
   }
  }
}

  /**
   *  ��ȡ�ļ�����
   * @param handle  �ļ���� 
   * @param *buf    �������ݵĻ��� 
   * @param bytes   ��ȡ�����ݳ���
   * @returns ���ض�ȡ���ݳ���   С��0�����
   */
static int Read_File(void* handle, void* buf, int bytes)
{  
  Iap_FlashCont * Filehandle = (Iap_FlashCont *) handle ;
  printf("��ȡ�ļ����� ��ȡ���ȣ� %ld \r\n",bytes);    
  if(!Filehandle->InitFlat)  //δ��ʼ��
  {
    return ERR_MEM ;
  } 
  uint16_t Count ;
  for(  Count = 0 ; (Count < bytes)&&(Filehandle->Flash_Addr<=FLASH_END)  ;  Count++ ,Filehandle->Flash_Addr++ )
  {
   ((uint8_t *)buf)[Count] = *((__IO uint8_t *) Filehandle->Flash_Addr);  
  }
  return Count;
}


  /**
   *  д�ļ�����
   * @param handle           �ļ����
   * @param struct pbuf* p   ���ݻ���ṹ��  ��������ݻ���ȫΪʵ����Ҫд�������
   * @returns С��0Ϊ����  
   */
static int Write_File(void* handle, struct pbuf* p)
{
  uint16_t Count ;
  Iap_FlashCont * Filehandle = (Iap_FlashCont *) handle ;     
  printf("д�ļ�����  ���ݳ��� %ld \r\n",p->len); 
  if(!Filehandle->InitFlat)
  {
    printf("д�ļ�  û�г�ʼ�� \r\n");
    return ERR_MEM ;
  }
  Count = p->len/4 +((p->len%4)>0);  //�õ�Ҫд�������
  printf("��ʼдFLASH  ��ַ ��0X%08X \r\n",Filehandle->Flash_Addr);
  if( FLASH_If_Write((__IO uint32_t*)&Filehandle->Flash_Addr,(uint32_t *)p->payload,Count) == 0 )
  {
    printf("дFLASH�ɹ�  ��һ�ε�ַ ��0X%08X \r\n",Filehandle->Flash_Addr);
    return ERR_OK;
  }
  else
  {
    printf("дFLASH ʧ�� �����ַ �� 0X%08X \r\n",Filehandle->Flash_Addr);
  }
  return ERR_MEM;
}

/* For TFTP client only */
static void
Tftp_Error(void* handle, int err, const char* msg, int size)
{
  char message[100];

  LWIP_UNUSED_ARG(handle);

  memset(message, 0, sizeof(message));
  MEMCPY(message, msg, LWIP_MIN(sizeof(message)-1, (size_t)size));

  printf("TFTP error: %d (%s)", err, message);
}

void
tftp_example_init_client(void)
{
  void *f;
  err_t err;
  ip_addr_t srv;
  int ret = ipaddr_aton(LWIP_TFTP_EXAMPLE_CLIENT_REMOTEIP, &srv);
  LWIP_ASSERT("ipaddr_aton failed", ret == 1);

  err = tftp_init_client(&tftpContext);
  LWIP_ASSERT("tftp_init_client failed", err == ERR_OK);

  f = tftp_open_file(LWIP_TFTP_EXAMPLE_CLIENT_FILENAME, 1);
  LWIP_ASSERT("failed to create file", f != NULL);

  err = tftp_get(f, &srv, TFTP_PORT, LWIP_TFTP_EXAMPLE_CLIENT_FILENAME, TFTP_MODE_OCTET);
  LWIP_ASSERT("tftp_get failed", err == ERR_OK);
}



