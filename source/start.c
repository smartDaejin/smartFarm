#include <wiringPi.h>
#include <wiringPiSPI.h>
#include <mcp3004.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <signal.h>

#define BASE 100
#define SPI_CHAN 0

#define MAXTIMINGS 85
#define DHTPIN 23

#define input_p1 24 // 펌프 기본값 1
#define input_p2 25 // 펌프 기본값 0 작동 1 정지

#define input_f1 28 // 팬 기본값 1
#define input_f2 29 // 팬 기본값 0 작동 1 정지

int DHT11[5] = {0, 0, 0, 0, 0};

void sig_Handler(int sig);
void init(void);

void read_dht11()
{
}

void soil()
{
    printf("현재 토양습도 : %d \n", analogRead(BASE + 2));
    printf("토양 습도 기준치 이하 펌프작동\n");
    digitalWrite(input_p2, 0);
    delay(2000);
    while (400 >= analogRead(BASE + 2))
    {
        digitalWrite(input_p2, 1);
    }
    if (400 >= analogRead(BASE + 2))
    {
        printf("펌프 정지\n");
        delay(2000);
    }
}

/*
void control()
{
    char str[10]; // 입력값 문자열

    while (1)
    {
        printf("입력 모드 진입\n");
        scanf(" %s", str);

        if (strcmp(str, "fanon") == 0)
        {
            digitalWrite(input_f2, 0);
            printf("팬작동\n");
        }
        else if (strcmp(str, "fanoff") == 0)
        {
            digitalWrite(input_f2, 1);
            printf("팬정지\n");
        }
        else if (strcmp(str, "pumpon") == 0)
        {
            digitalWrite(input_p2, 0);
             printf("펌프작동\n");
        }
        else if (strcmp(str, "pumpoff") == 0)
        {
            digitalWrite(input_p2, 1);
            printf("펌프정지\n");
        }
        else
        {
            printf("제대로 입력해주세요.\n");
        }
    }

}
*/

int main(void)
{
    // wiringPi, SPI setup
    if (wiringPiSetup() == -1)
    {
        return -1;
    }

    printf("wiringPiSPISetup return=%d\n", wiringPiSPISetup(0, 500000)); // SPI 채널번호 (0 ~ 2), 속도
    mcp3004Setup(BASE, SPI_CHAN);

    signal(SIGINT, sig_Handler); // 시그널 핸들러 함수 정의

    pinMode(input_p1, OUTPUT); // 워터펌프
    pinMode(input_p2, OUTPUT);

    pinMode(input_f1, OUTPUT); // 쿨링팬
    pinMode(input_f2, OUTPUT);

    digitalWrite(input_f1, 1); // 기본값 지정 1, 1 멈춤 1, 0 작동
    digitalWrite(input_f2, 0); // 팬작동

    digitalWrite(input_p1, 1); // 기본값 지정 1, 1 멈춤 1, 0 작동
    digitalWrite(input_p2, 1);

    while (1) // 센서 작동
    {
        uint8_t laststate = HIGH;
        uint8_t counter = 0;
        uint8_t j = 0, i;
        float f;

        DHT11[0] = DHT11[1] = DHT11[2] = DHT11[3] = DHT11[4] = 0;

        pinMode(DHTPIN, OUTPUT);    /*	DHTPIN의 출력 설정*/
        digitalWrite(DHTPIN, LOW);  /*	LOW(0) 값을 출력:	DHT11 끄기	*/
        delay(18);                  /*	00.18초(18밀리초) 동안 대기	*/
        digitalWrite(DHTPIN, HIGH); /*	HIGH(1) 값을 출력:	DHT11 켜기	*/
        delayMicroseconds(40);
        pinMode(DHTPIN, INPUT); /*	DHTPIN 모드 입력으로 설정	*/

        for (i = 0; i < MAXTIMINGS; i++)
        {
            counter = 0;
            while (digitalRead(DHTPIN) == laststate) /*	DHT11이 켜져 있는가?	*/
            {
                counter++;
                delayMicroseconds(1);
                if (counter == 255)
                {
                    break;
                }
            }
            laststate = digitalRead(DHTPIN); /*	DHT11상태 laststate에 저장	*/

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
            (DHT11[4] == ((DHT11[0] + DHT11[1] + DHT11[2] + DHT11[3]) & 255))) /*	DHT11 측정된 값이 40보다 크거나 같고 255보다 작은가?	*/
        {
            f = DHT11[2] * 9. / 5. + 32;
            printf("습기 = %d.%d %% 온도 = %d.%d C (%.1f F)\n",
                   DHT11[0], DHT11[1], DHT11[2], DHT11[3], f);
        }
        else
        {
            printf("온습도가 측정되지 않았습니다.\n");
        }
        if (400 <= analogRead(BASE + 2)) // 토양 습도 낮을시 동작
        {
            delay(3000);
            soil();
        }
        delay(2000);
    }
}

// signal handler function
void sig_Handler(int sig)
{

    printf("\n\n종료\n\n");
    init();
    exit(0);
}

void init(void)
{
    digitalWrite(input_f2, 1);
    digitalWrite(input_p2, 1);
}
