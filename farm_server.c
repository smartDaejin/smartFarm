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

void all_stop_func()
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
 *  main - 프로그램 실행 및 센서 동작 함수
 *
 *  모든 센서를 동작하는 코드를 가지고 있습니다.
 */
int main(void)
{
    // wiringPi, SPI setup
    if (wiringPiSetup() == -1)
    {
        return -1;
    }

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

    return 0;
}