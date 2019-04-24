#include <avr/io.h> //The header file for Input&OutPut in AVR
#include <util/delay.h> // The header file for Delay Function
#include "lcd.h" //The header file for Text LCD

#include "TWI_driver.h" // TWI 사용을 위한 헤더 파일
#include "SHT2x.h" // SHT2x 라이브러리 사용을 위한 헤더 파일

#include <avr/interrupt.h>
volatile unsigned char Time_STOP = 0;     // 스위치0 인터럽트 헤더파일
volatile unsigned char Time_STOP1 = 0;     // 스위치1 인터럽트 헤더파일


#define M1_Forward 0x10
#define M1_Reverse 0x20
#define M1_Enable 0x20

void printf_2dot1(uint8_t sense,uint16_t sense_temp);
// 온도 및 습도를 LCD에 출력하는 함수
uint16_t temperatureC,humidityRH;
// 온도, 습도 값 측정에 사용되는 변수


int main(void)
{
	
	
	uint8_t error = 0; // 에러를 저장하는 변수
	nt16 sRH; // 습도의 raw 데이터를 저장하는 변수
	nt16 sT; // 온도의 raw 데이터를 저장하는 변수
	Init_TWI(); // TWI를 초기화
	lcdInit(); // Text LCD를 초기화
	SHT2x_Init(); // SHT 센서를 초기화
	
	unsigned int DoReMi[8] = {523,587, 659, 698, 783, 1046};
	unsigned int AdData =0; //10bit variable for 'ADC'
	//float v_temp; //Variable for Value of Voltage

	ADMUX=0x01; // ACD Multiplexer Selection Register, 0000 0001, Chose ADC1
	
	ADCSRA=0x87; // A/D Converter Control and Status Register A, 1000 0111
	//permit 'ADC', Prescaler at 128
	//----------------------------------------------------------------------
	
	//DDRB =0x80; // PWM OUTPUT, OCR1C
	TCCR1A |= 0x8A;
	TCCR1B |= 0x19;
	TCCR1C =0x00;
	TCNT1 =0x0000;
	//----------------------------------------------------------------------
	DDRD =0x30;
	DDRB = 0xA0;
	//-----------------------------------스위치 인터럽트----------------------
	DDRE = 0x00; // 포트E 를 입력포트로 설정
	EIMSK=0x30;
	EICRB=0x0F;
	EIFR=0x30;
	sei();
	//----------------------------------------------------------------------
	
	
	//PORTD=M1_Forward;

	while (1)
	{
		//Read 'ADC' through ADC1
		
		
		switch(Time_STOP){
			case 0:
			DDRB=0x20;
		error |= SHT2x_MeasureHM(HUMIDITY, &sRH); // 습도를 측정
		error |= SHT2x_MeasureHM(TEMP, &sT); // 온도를 측정
		// 온도 습도를 계산, 소숫점 첫째자리까지 출력하기 위해 10을 곱함
		temperatureC = SHT2x_CalcTemperatureC(sT.u16)*10;// 온도를 계산
		humidityRH = SHT2x_CalcRH(sRH.u16)*10; // 습도를 계산
		ICR1=12000;
		if(error == SUCCESS) { // 에러없이 정상 측정일 경우
			//lcdClear(); // Text LCD를 초기화
			lcdGotoXY(0,0); // 커서위치를 첫번째 줄, 첫번째 칸으로 이동
			lcdPrintData("Auto Mode ",10);
			lcdPrintData("65/75:",5);
			//printf_2dot1(TEMP,temperatureC); // 온도를 출력
			lcdGotoXY(0,1); // 커서위치를 두번째 줄, 첫번째 칸으로 이동
			printf_2dot1(HUMIDITY,humidityRH); // 습도를 출력
			
			if(humidityRH>=650){
			//ICR1= 14745600/DoReMi[0];
			
			OCR1A=ICR1/4; //Duty Ratio 50%
			PORTD=M1_Reverse;
			_delay_ms(100);
			}
			if(humidityRH>=750)
			{
				//ICR1= 14745600/DoReMi[6];
				
				OCR1A=ICR1; //Duty Ratio 50%
				PORTD=M1_Reverse;
				_delay_ms(100);
			}
			if(humidityRH<=650)
			{
				PORTD|=0x30;
			}
		}
		else { // 에러가 있을 경우
			lcdGotoXY(0,0); // 커서위치를 첫번째 줄, 첫번째 칸으로 이동
			lcdPrintData(" Temp: --.-C   ",15); // 온도를 --.-로 출력
			lcdGotoXY(0,1); // 커서위치를 두번째 줄, 첫번째 칸으로 이동
			lcdPrintData(" Humi: --.-%",12); // 습도를 --.-로 출력
		}
		_delay_ms(300); // 다음 측정을 위한 시갂 지연(300ms)
	
		break;
		
		
		case 1:
		ADCSRA |= 0x40;   // 0100 0000
		while((ADCSRA & 0x10)==0x00); //ADIF AD
		AdData=ADC;
		//lcdClear(); // Text LCD를 초기화
		lcdGotoXY(0,0); // 커서위치를 첫번째 줄, 첫번째 칸으로 이동
		lcdPrintData(" Temp: --.-C    ",16); // 온도를 --.-로 출력
		lcdGotoXY(0,1); // 커서위치를 두번째 줄, 첫번째 칸으로 이동
		lcdPrintData(" Humi: --.-%",12); // 습도를 --.-로 출력
		
		if(AdData<=128)
		{

			
			ICR1= 0;
			PORTD|=0x30;
		}
		else if(AdData<=256)
		{
			
			
			ICR1= 14745600/DoReMi[0];
			OCR1C=ICR1/2; //Duty Ratio 50% PIEZO
			OCR1A=ICR1/7; //Duty Ratio 50% MOTOR
			PORTD=M1_Reverse;
			_delay_ms(1000);
			
		}
		else if(AdData<=400)
		{
			
			ICR1= 14745600/DoReMi[1];
			OCR1C=ICR1/2; //Duty Ratio 50%
			OCR1A=ICR1/7; //Duty Ratio 50%
			PORTD=M1_Reverse;
			_delay_ms(1000);
		}
		else if(AdData<=512)
		{
			
			ICR1= 14745600/DoReMi[2];
			OCR1C=ICR1/2; //Duty Ratio 50%
			OCR1A=ICR1/4; //Duty Ratio 50%
			PORTD=M1_Reverse;
			_delay_ms(1000);
			
		}
		else if(AdData<=640)
		{
			
			ICR1= 14745600/DoReMi[3];
			OCR1C=ICR1/2; //Duty Ratio 50%
			OCR1A=ICR1/4; //Duty Ratio 50%
			PORTD=M1_Reverse;
			_delay_ms(1000);
		}
		else if(AdData<=768)
		{
			
			ICR1= 14745600/DoReMi[4];
			OCR1C=ICR1/2; //Duty Ratio 50%
			OCR1A=ICR1/2; //Duty Ratio 50%
			PORTD=M1_Reverse;
			_delay_ms(1000);
			
		}
		else if(AdData<=896)
		{
			
			ICR1= 14745600/DoReMi[5];
			OCR1C=ICR1/2; //Duty Ratio 50%
			OCR1A=ICR1/2; //Duty Ratio 50%
			PORTD=M1_Reverse;
			_delay_ms(1000);
			
		}
		else if(AdData<=1024)
		{
			
			ICR1= 14745600/DoReMi[6];
			OCR1C=ICR1/2; //Duty Ratio 50%
			OCR1A=ICR1; //Duty Ratio 50%
			PORTD=M1_Reverse;
			_delay_ms(1000);
			}
			break;
		}
	}
		//----------------------------------------------------------------------
		
		//----------------------------------------------------------------------
		_delay_ms(10);
}


void printf_2dot1(uint8_t sense,uint16_t sense_temp) {
	uint8_t s100,s10;
	//if(sense == TEMP) lcdPrintData(" Temp: ",7); // 온도 출력
	 if(sense == HUMIDITY) lcdPrintData(" Humi: ",7);// 습도 출력
	s100 = sense_temp/100; // 100의 자리 추출
	if(s100> 0) lcdDataWrite(s100+'0'); // 100의 자리 값이 있으면 출력
	else lcdPrintData(" ",1); // 100의 자리 값이 없으면 빈칸 출력
	s10 = sense_temp%100; // 100의 자리를 제외한 나머지 추출
	lcdDataWrite((s10/10)+'0'); // 10의 자리 추출하여 출력
	lcdPrintData(".",1); // 소숫점 출력
	lcdDataWrite((s10%10)+'0'); // 1의 자리 추출하여 출력
	//if(sense == TEMP) lcdDataWrite('C'); // 온도 단위 출력
	if(sense == HUMIDITY) lcdDataWrite('%'); // 습도 단위 출력
}

ISR(INT4_vect) // 인터럽트 서비스 루틴
{
	cli(); // 젂체 인터럽트를 금지
	if(Time_STOP == 0) // Time_Stop이 0인 경우
	{
		Time_STOP = 1; // Time_Stop에 1을 입력
	}
	else // Time_Stop이 1인 경우
	{
		Time_STOP = 0; // Time_Stop에 0을 입력
	}
	sei(); // 젂체 인터럽트를 허용
}

ISR(INT5_vect) // 인터럽트 서비스 루틴
{
	cli(); // 젂체 인터럽트를 금지
	if(Time_STOP1 == 0) // Time_Stop이 0인 경우
	{
		Time_STOP1 = 1; // Time_Stop에 1을 입력
	}
	else // Time_Stop이 1인 경우
	{
		Time_STOP1 = 0; // Time_Stop에 0을 입력
	}
	sei(); // 젂체 인터럽트를 허용
}
