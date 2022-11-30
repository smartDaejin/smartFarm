#include <wiringPi.h>
#include <wiringPiSPI.h>
#include <mcp3004.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>


#define BASE 100
#define SPI_CHAN 0 

#define MAXTIMINGS    85
#define DHTPIN        23

#define input_p1    24    // 펌프 기본값 1
#define input_p2    25    // 펌프 기본값 0 작동 1 정지


#define input_f1    28 // 팬 기본값 1
#define input_f2    29 // 팬 기본값 0 작동 1 정지 
int main(void)
{

if (wiringPiSetup() == -1)
    {
        return -1;
    }

    printf("wiringPiSPISetup return=%d\n", wiringPiSPISetup(0, 500000));  // SPI 채널번호 (0 ~ 2), 속도
	mcp3004Setup(BASE, SPI_CHAN);

    pinMode( input_p1, OUTPUT ); // 워터펌프
    pinMode( input_p2, OUTPUT );

    pinMode( input_f1, OUTPUT ); // 쿨링팬
    pinMode( input_f2, OUTPUT );
    
    pinMode( DHTPIN, INPUT );	


    digitalWrite(input_f1, 1); // 기본값 지정 1, 1 멈춤 1, 0 작동
    digitalWrite(input_f2, 1); // 팬정지

    digitalWrite(input_p1, 1); // 기본값 지정 1, 1 멈춤 1, 0 작동 
    digitalWrite(input_p2, 1); // 펌프정지
    digitalWrite( DHTPIN, LOW );	
    printf("종료\n");
}