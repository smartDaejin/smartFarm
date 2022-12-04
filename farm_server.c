/*
 *  smart farm 센서 정보 수집 및 제어 서버 프로그램
 *
 * 	목표:
 *		smart farm에 사용되는 센서 정보를 수집하고 센서를 제어하는 웹 서버를 구성한다.
 *	구성:
 *		센서 모듈로부터 온도, 습도, 토양 습도 데이터를 반복적으로 수집
 *  	특정 조건에 워터 펌프 센서 작동
 *  	팬쿨러는 계속 실행
 *      웹 서버를 통한 센서 수집 및 정지를 제어한다.
 *	작성자:
 *		서민원	<20202170>
 *		조윤준	<20202177>
 */

#include <wiringPi.h>
#include <wiringPiSPI.h>
#include <mcp3004.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <linux/input.h>

#define BASE 100
#define SPI_CHAN 0

#define MAXTIMINGS 85
#define DHTPIN 23

#define input_p1 24 /* 펌프 기본값 1 */
#define input_p2 25 /* 펌프 기본값 0 작동 1 정지 */

#define input_f1 28 /* 팬 기본값 1 */
#define input_f2 29 /* 팬 기본값 0 작동 1 정지 */

int DHT11[5] = {0, 0, 0, 0, 0};


/*
 *  all_stop_func - 모든 센서 종료 함수
 *
 *  모든 센서 동작을 종료합니다.
 */

void all_sensor_stop()
{
    printf("wiringPiSPISetup return=%d\n", wiringPiSPISetup(0, 500000));    /* SPI 채널번호 (0 ~ 2), 속도 */
	mcp3004Setup(BASE, SPI_CHAN);

    pinMode( input_p1, OUTPUT );    /* 워터펌프 */
    pinMode( input_p2, OUTPUT );

    pinMode( input_f1, OUTPUT );    /* 쿨링팬 */
    pinMode( input_f2, OUTPUT );
    
    pinMode( DHTPIN, INPUT );	


    digitalWrite(input_f1, 1);  /* 기본값 지정 1, 1 멈춤 1, 0 작동 */
    digitalWrite(input_f2, 1);  /* 팬정지 */

    digitalWrite(input_p1, 1);  /* 기본값 지정 1, 1 멈춤 1, 0 작동 */
    digitalWrite(input_p2, 1);  /* 펌프정지 */
    digitalWrite( DHTPIN, LOW );	
    printf("종료\n");
}

/*
 *  run_water_pump - 워터펌프 작동 함수
 *
 *  토양 습도 기준치 이하일 경우 물을 줄기 위해 워터펌프를 작동하는 함수
 */
void run_water_pump()
{
    printf("현재 토양습도 : %d \n", analogRead(BASE + 2));
    printf("토양 습도 기준치 이하 펌프작동\n");
    digitalWrite(input_p2, 0);
    delay(2000);
    while (400 >= analogRead(BASE + 2))
    {
        digitalWrite(input_p2, 1);
        break;
    }
    if (400 >= analogRead(BASE + 2))
    {
        printf("펌프 정지\n");
        delay(2000);
    }
}

/*
 *  collection_senser_start - 측정 센서 동작 함수
 *
 *  모든 측정 센서를 동작하는 코드를 가지고 있습니다.
 */
void collection_senser_start()
{
    printf("wiringPiSPISetup return=%d\n", wiringPiSPISetup(0, 500000)); /* SPI 채널번호 (0 ~ 2), 속도 */
    mcp3004Setup(BASE, SPI_CHAN);

    pinMode(input_p1, OUTPUT); /* 워터펌프 */
    pinMode(input_p2, OUTPUT);

    pinMode(input_f1, OUTPUT); /* 쿨링팬 */
    pinMode(input_f2, OUTPUT);

    digitalWrite(input_f1, 1); /* 기본값 지정 1, 1 멈춤 1, 0 작동 */
    digitalWrite(input_f2, 0); /* 팬작동 */

    digitalWrite(input_p1, 1);  /* 기본값 지정 1, 1 멈춤 1, 0 작동 */
    digitalWrite(input_p2, 1);

    while (1)   /* 센서 작동 */
    {
        uint8_t laststate = HIGH;
        uint8_t counter = 0;
        uint8_t j = 0, i;
        float f;

        DHT11[0] = DHT11[1] = DHT11[2] = DHT11[3] = DHT11[4] = 0;

        pinMode(DHTPIN, OUTPUT);    /* DHTPIN의 출력 설정 */
        digitalWrite(DHTPIN, LOW);  /* LOW(0) 값을 출력:	DHT11 끄기	*/
        delay(18);                  /* 00.18초(18밀리초) 동안 대기	*/
        digitalWrite(DHTPIN, HIGH); /* HIGH(1) 값을 출력:	DHT11 켜기 */
        delayMicroseconds(40);
        pinMode(DHTPIN, INPUT); /* DHTPIN 모드 입력으로 설정 */

        for (i = 0; i < MAXTIMINGS; i++)
        {
            counter = 0;
            while (digitalRead(DHTPIN) == laststate)    /*	DHT11이 켜져 있는가? */
            {
                counter++;
                delayMicroseconds(1);
                if (counter == 255)
                {
                    break;
                }
            }
            laststate = digitalRead(DHTPIN);    /*	DHT11상태 laststate에 저장	*/

            if (counter == 255)
                break;

            if ((i >= 4) && (i % 2 == 0))
            {
                DHT11[j / 8] <<= 1;
                if (counter > 50)
                    DHT11[j / 8] |= 1;
                j++;
            }
        }

        if ((j >= 40) &&
            (DHT11[4] == ((DHT11[0] + DHT11[1] + DHT11[2] + DHT11[3]) & 255)))  /* DHT11 측정된 값이 40보다 크거나 같고 255보다 작은가? */
        {
            f = DHT11[2] * 9. / 5. + 32;
            printf("습기 = %d.%d %% 온도 = %d.%d C (%.1f F)\n",
                   DHT11[0], DHT11[1], DHT11[2], DHT11[3], f);
        }
        else
        {
            printf("온습도가 측정되지 않았습니다.\n");
        }
        if (400 <= analogRead(BASE + 2))    /* 토양 습도 낮을시 동작 */
        {
            delay(3000);
            run_water_pump();
        }
        delay(2000);
    }
}

/*
 *  main - 프로그램 실행 및 센서 동작 함수
 *
 *  모든 센서를 동작하는 코드를 가지고 있습니다.
 */
int main(int argc, char **argv)
{
    int serv_sock;
    pthread_t tid;
    struct sockaddr_in serv_addr, clnt_addr;
    socklen_t clnt_addr_size;

     /* 서버를 위한 소켓을 생성한다. */
    serv_sock = socket(AF_INET, SOCK_STREAM, 0);
    if(serv_sock == -1) {
        perror("socket()");
        exit(1);
    }

    /* 입력받는 포트 번호를 이용해서 서비스를 운영체제에 등록한다. */
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(atoi(argv[1]));
    if(bind(serv_sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("bind");
        exit(1);
    }

    /* 프로그램을 시작할 때 서버를 위한 포트 번호를 입력받는다. */
    if(argc!=2) {
        printf("usage: %s <port>\n", argv[0]);
        return -1;
    }

    /* wiringPi, SPI setup */
    if (wiringPiSetup() == -1)
    {
        return -1;
    }

    /* 최대 10대의 클라이언트의 동시 접속을 처리할 수 있도록 큐를 생성한다. */
    if(listen(serv_sock, 10) < 0) {
        perror("listen");
        return -1;
    }

    while(1) {
        int clnt_sock;

        /* 클라이언트의 요청을 기다린다. */
        clnt_addr_size = sizeof(clnt_addr);
        clnt_sock = accept(serv_sock, (struct sockaddr *)&clnt_addr, &clnt_addr_size);

        /* 네트워크 주소를 문자열로 변경 */
        printf("Client IP : %s:%d\n", inet_ntoa(clnt_addr.sin_addr), ntohs(clnt_addr.sin_port));

        /* 클라이언트의 요청이 들어오면 스레드를 생성하고 클라이언트의 요청을 처리한다. */
        pthread_create(&tid, NULL, clnt_connection, (void *)&clnt_sock);
        pthread_detach(tid);
    }

    return 0;
}

void *clnt_connection(void *arg)
{
    /* 스레드를 통해서 넘어온 arg를 int 형의 파일 디스크립터로 변환한다. */
    int clnt_sock = *((int *)arg), clnt_fd;
    FILE *clnt_read, *clnt_write;
    
    char reg_line[BUFSIZ], reg_buf[BUFSIZ];
    char method[10];
    char file_name[256];
    
    /* 파일 디스크립터를 FILE 스트림으로 변환한다. */
    clnt_read = fdopen(clnt_sock, "r");
    clnt_write = fdopen(dup(clnt_sock), "w");
    clnt_fd = clnt_sock;

    /* 한 줄의 문자열을 읽어서 reg_line 변수에 저장한다. */
    fgets(reg_line, BUFSIZ, clnt_read);

    /* reg_line 변수에 문자열을 화면에 출력한다. */
    fputs(reg_line, stdout);

     /* ' ' 문자로 reg_line을 구분해서 요청 라인의 내용(메소드)를 분리한다. */
    strcpy(method, strtok(reg_line, " /"));
    if(strcmp(method, "POST") == 0) {   /* POST 메소드일 경우를 처리한다. */
        sendOk(clnt_write);             /* 단순히 OK 메시지를 클라이언트로 보낸다. */
        fclose(clnt_read);
        fclose(clnt_write);
        return NULL;
    }
    else if(strcmp(method, "GET") != 0) {   /* GET 메소드가 아닐 경우를 처리한다. */
        sendError(clnt_write);              /* 에러 메시지를 클라이언트로 보낸다. */
        fclose(clnt_read);
        fclose(clnt_write);
        return NULL;
    }
    
    strcpy(file_name, strtok(NULL, " /"));  /* 요청 라인에서 경로(path)를 가져온다. */
    printf("file_name : %s\n", file_name);
    if(strstr(file_name, "?") != NULL) {
        // LED 버튼을 누르고 submit을 했다면 ?led=On 혹은 ?led=Off라고 발송된다
        char opt[8], var[8];
        strtok(file_name, "?");
        // led와 On/Off 분리
        strcpy(opt, strtok(NULL, "="));
        strcpy(var, strtok(NULL, "="));
        
        printf("%s=%s\n", opt, var);
        if(!strcmp(opt, "led") && !strcmp(var, "On")) {
            ledControl(LED, 1);
        } else if(!strcmp(opt, "led") && !strcmp(var, "Off")) {
            ledControl(LED, 0);   
        }
    }
    
    /* 메시지 헤더를 읽어서 화면에 출력하고 나머지는 무시한다. */
    do {
        fgets(reg_line, BUFSIZ, clnt_read);
        fputs(reg_line, stdout);
        strcpy(reg_buf, reg_line);
    } while(strncmp(reg_line, "\r\n", 2));  /* 요청 헤더는 ‘\r\n’으로 끝난다. */
    
    /* 파일의 이름을 이용해서 클라이언트로 파일의 내용을 보낸다. */
    sendData(clnt_fd, clnt_write, file_name);
    
    fclose(clnt_read);  /* 파일의 스트림을 닫는다. */
    fclose(clnt_write);
    return NULL;
}

int sendData(int fd, FILE *fp, char *file_name)
{
    /* 클라이언트로 보낼 성공에 대한 응답 메시지 */
    char protocol[] = "HTTP/1.1 200 OK\r\n";
    char server[] = "Server:Netscape-Enterprise\6.0\r\n";
    char cnt_type[] = "Content-Type:text/html\r\n";
    char end[] = "\r\n";    /* 응답 헤더의 끝은 항상 \r\n */
    char buf[BUFSIZ];
    int len;

    fputs(protocol, fp);
    fputs(server, fp);
    fputs(cnt_type, fp);
    fputs(end, fp);
    fflush(fp);

    fd = open(file_name, O_RDWR);
    do {
        len = read(fd, buf, BUFSIZ);
        fwrite(buf, len, sizeof(char), fp);
    } while(len == BUFSIZ);

    fflush(fp);
    close(fd);

    return 0;
}

void sendOk(FILE* fp)
{
    /* 클라이언트에 보낼 성공에 대한 HTTP 응답 메시지 */
    char protocol[ ] = "HTTP/1.1 200 OK\r\n";
    char server[ ] = "Server: Netscape-Enterprise/6.0\r\n\r\n";

    fputs(protocol, fp);
    fputs(server, fp);
    fflush(fp);
}
    
void sendError(FILE* fp)
{
    /* 클라이언트로 보낼 실패에 대한 HTTP 응답 메시지 */
    char protocol[ ] = "HTTP/1.1 400 Bad Request\r\n";
    char server[ ] = "Server: Netscape-Enterprise/6.0\r\n";
    char cnt_len[ ] = "Content-Length:1024\r\n";
    char cnt_type[ ] = "Content-Type:text/html\r\n\r\n";

    /* 화면에 표시될 HTML의 내용 */
    char content1[ ] = "<html><head><title>BAD Connection</title></head>";
    char content2[ ] = "<body><font size=+5>Bad Request</font></body></html>";
    printf("send_error\n");

    fputs(protocol, fp);
    fputs(server, fp);
    fputs(cnt_len, fp);
    fputs(cnt_type, fp);
    fputs(content1, fp);
    fputs(content2, fp);
    fflush(fp);
}