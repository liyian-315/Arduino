#include <stdio.h> /*标准输入输出定义*/
#include <stdlib.h> /*标准函数库定义*/
#include <unistd.h> /*Unix 标准函数定义*/
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h> /*文件控制定义*/
#include <termios.h> /*POSIX 终端控制定义*/
#include <errno.h> /*错误号定义*/
#include <string.h> /*字符串功能函数*/
#include <sys/time.h>
#include <sys/types.h>
#include <mysql/mysql.h>

#define SERIAL_PORT "/dev/ttyUSB0"
#define DB_HOST "127.0.0.1"
#define DB_USER "root"
#define DB_PASS "123"
#define DB_NAME "mydatabase"

int tty_fd = -1;
char r_buf[128];
struct termios options;
fd_set rset;
_Bool isOpne = 0;
long num=0;
MYSQL *conn;

void close_serial() {
printf("close_seria ===============\n\n");
isOpne = 1;
close(tty_fd);
}

void open_serial_init() {
int rv = -1;
tty_fd = open(SERIAL_PORT, O_RDWR | O_NOCTTY); //打开串口设备
if (tty_fd < 0)
{
printf("open tty failed:%s\n", strerror(errno));
close_serial();
return;
}

printf("open devices sucessful!\n");
memset(&options, 0, sizeof(options));
rv = tcgetattr(tty_fd, &options); //获取原有的串口属性的配置
if (rv != 0)
{
printf("tcgetattr() failed:%s\n", strerror(errno));
close_serial();
return;
}

options.c_cflag |= (CLOCAL | CREAD); // CREAD 开启串行数据接收，CLOCAL并打开本地连接模式
options.c_cflag &= ~CSIZE;// 先使用CSIZE做位屏蔽
options.c_cflag |= CS8; //设置8位数据位
options.c_cflag &= ~PARENB; //无校验位


/* 设置波特率 */
cfsetispeed(&options, B115200);
cfsetospeed(&options, B115200);
options.c_cflag &= ~CSTOPB;/* 设置一位停止位; */
options.c_cc[VTIME] = 10;/* 非规范模式读取时的超时时间；*/
options.c_cc[VMIN] = 0; /* 非规范模式读取时的最小字符数*/
tcflush(tty_fd, TCIFLUSH);/* tcflush清空终端未完成的输入/输出请求及数据；TCIFLUSH表示清空正收到的数据，且不读取出来 */


if ((tcsetattr(tty_fd, TCSANOW, &options)) != 0)
{
printf("tcsetattr failed:%s\n", strerror(errno));
close_serial();
return;
}
}


int mysqlInit()
{
    conn=mysql_init(NULL);
    if(!mysql_real_connect(conn,
    DB_HOST,
    DB_USER,
    DB_PASS,
    DB_NAME,
    0,
    NULL,
    0
    ))
    {
        fprintf(stderr,"无法连接数据库: %s\n", mysql_error(conn));
        close_serial();
        return -1;
    }
    else{
        printf("mysql coonect ok!\n");
        return 1;
    }

}



void read_serial() {
int rv = -1;
while (1) {
if (isOpne == 0) {
memset(r_buf, 0, sizeof(r_buf));
rv = read(tty_fd, r_buf, sizeof(r_buf));
if (rv < 0)
{
printf("Read() error:%s\n", strerror(errno));
close_serial();
return;
}
else {
//打印数据,,字符流转为int

int i = 0;
while (i < rv) {
    if(r_buf[i]>='0'&&r_buf[i]<='9')
    {
        num=num*10+r_buf[i]-48;
    }
    if(r_buf[i]=='\n')
    {
        printf("%ld",num);
        //向mysql插入数据
        char query[512];
        sprintf(query, "insert into mydata (data) values (%ld)",num);
        if(mysql_query(conn,query))
        {
            fprintf(stderr,"%s\n",mysql_error(conn));
            mysql_close(conn);
            exit;
        }
        num=0;
    }    
printf("%c", r_buf[i]);
i++;
    

}

//printf("%s",r_buf);
if (0x15 == r_buf[0]) {
printf("ttyUSB0 is live!\n");
break;
}
}
usleep(5000);
}
else {
printf("Read tty null=: %s\n", r_buf);
}
}
mysql_close(conn);
}

int main()
{
    open_serial_init();
    mysqlInit();
    read_serial();
    return 0;
}

/*
void write_serial(char* buff, int size) {
int rv = -1;
rv = write(tty_fd, buff, size);
printf("write_serial rv=============== size=%d\n", rv);
if (rv < 0)
{
printf("Write() error:%s\n", strerror(errno));
close_serial();
return;
}

}
*/
