/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Main program body
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2026 STMicroelectronics.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************
 */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "tim.h"
#include "usb_device.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdbool.h>
#include <string.h>
#include <stdint.h>
#include "usbd_custom_hid_if.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define SERVO_CENTER_US 1500
#define SERVO_MIN_US 518
#define SERVO_MAX_US 2482

#define HID_OUT_LEN 64
#define HID_IN_LEN 64

#define MAX_ANGLE 135
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
extern USBD_HandleTypeDef hUsbDeviceFS;
extern volatile uint8_t g_hid_rx_flag;
extern volatile uint32_t g_hid_rx_len;
extern uint8_t g_hid_rx_buf[HID_OUT_LEN];
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
void servoSetPulseUs(uint16_t us) {
	if (us < SERVO_MIN_US)
		us = SERVO_MIN_US;
	if (us > SERVO_MAX_US)
		us = SERVO_MAX_US;

	__HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_3, us);
}

void servoSetAngle(int16_t angle_deg) {
	// [-135, 135]
	if (angle_deg < -MAX_ANGLE)
		angle_deg = -MAX_ANGLE;
	if (angle_deg > MAX_ANGLE)
		angle_deg = MAX_ANGLE;

	int32_t span = (SERVO_MAX_US - SERVO_CENTER_US);
	int32_t us = SERVO_CENTER_US + ((int32_t) angle_deg * span) / MAX_ANGLE;

	// [0, 270]
	//	if (angle_deg < 0 || angle_deg > 270) {
	//		return;
	//	}
	//	uint16_t us = SERVO_MIN_US + angle_deg / 270 * (SERVO_MAX_US - SERVO_MIN_US);
	servoSetPulseUs((uint16_t) us);
}

void sleep(uint16_t s) {
	HAL_Delay(s * 1000);
}

static inline int16_t clampI16(int16_t num, int16_t lo, int16_t hi) {
	if (num < lo)
		return lo;
	if (num > hi)
		return hi;

	return num;
}

typedef struct {
	uint8_t cmd;
	uint16_t mag;
} HidCmd;

static HidCmd hidReadCmd(void) {
	HidCmd out = { 0 };
	out.cmd = g_hid_rx_buf[0];
	out.mag = ((uint16_t) g_hid_rx_buf[1] << 8) | (uint16_t) g_hid_rx_buf[2];
	return out;
}

int16_t cmdToAngle(int16_t angle, uint8_t cmd, uint16_t mag) {
	switch (cmd) {
	case 0x00:
		angle = +(int16_t) mag;
		break;  // SET CCW
	case 0x01:
		angle = -(int16_t) mag;
		break;  // SET CW
	case 0x02:
		angle += (int16_t) mag;
		break;  // ADD CCW
	case 0x03:
		angle -= (int16_t) mag;
		break;  // ADD CW
	case 0x10:
		angle = 0;
		break;
	default: /* ignore unknown */
		break;
	}

	return clampI16(angle, -MAX_ANGLE, MAX_ANGLE);
}

void debug(HidCmd c) {
	static uint8_t tx[64];
	memset(tx, 0, sizeof(tx));
	tx[0] = c.cmd;
	tx[2] = c.mag & 0xFF;
	tx[1] = (c.mag >> 8) & 0xFF;
	tx[3] = g_hid_rx_len;

	USBD_CUSTOM_HID_SendReport(&hUsbDeviceFS, tx, 64);
}
/* USER CODE END 0 */

/**
 * @brief  The application entry point.
 * @retval int
 */
int main(void) {

	/* USER CODE BEGIN 1 */

	/* USER CODE END 1 */

	/* MCU Configuration--------------------------------------------------------*/

	/* Reset of all peripherals, Initializes the Flash interface and the Systick. */
	HAL_Init();

	/* USER CODE BEGIN Init */

	/* USER CODE END Init */

	/* Configure the system clock */
	SystemClock_Config();

	/* USER CODE BEGIN SysInit */

	/* USER CODE END SysInit */

	/* Initialize all configured peripherals */
	MX_GPIO_Init();
	MX_TIM2_Init();
	MX_USB_DEVICE_Init();
	/* USER CODE BEGIN 2 */

	HAL_GPIO_WritePin(RC_KEY_GPIO_Port, RC_KEY_Pin, GPIO_PIN_SET);

	HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_3);
//	__HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_3, 1500);
	servoSetAngle(0);
//	sleep(2);
//	servoSetAngle(-90);

	/* USER CODE END 2 */

	/* Infinite loop */
	/* USER CODE BEGIN WHILE */
	static int16_t target_angle = 0;
	static int16_t current_angle = 0;
	while (1) {

		// Protocol:
		// g_hid_rx_buf[0] = SET CCW (0) / SET CW (1); ADD CCW (2) / ADD CW (3);
		// g_hid_rx_buf[1] = angle in degree (large byte)
		// g_hid_rx_buf[2] = angle in degree (small byte)
		if (g_hid_rx_flag) {
			g_hid_rx_flag = 0;

			HidCmd c = hidReadCmd();
			target_angle = cmdToAngle(current_angle, c.cmd, c.mag);

			servoSetAngle(target_angle);

			current_angle = target_angle;

			debug(c);
		}
		/* USER CODE END WHILE */
	}

	/* USER CODE BEGIN 3 */

	/* USER CODE END 3 */
}

/**
 * @brief System Clock Configuration
 * @retval None
 */
void SystemClock_Config(void) {
	RCC_OscInitTypeDef RCC_OscInitStruct = { 0 };
	RCC_ClkInitTypeDef RCC_ClkInitStruct = { 0 };
	RCC_PeriphCLKInitTypeDef PeriphClkInit = { 0 };

	/** Initializes the RCC Oscillators according to the specified parameters
	 * in the RCC_OscInitTypeDef structure.
	 */
	RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
	RCC_OscInitStruct.HSEState = RCC_HSE_ON;
	RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
	RCC_OscInitStruct.HSIState = RCC_HSI_ON;
	RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
	RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
	RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
	if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
		Error_Handler();
	}

	/** Initializes the CPU, AHB and APB buses clocks
	 */
	RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
			| RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
	RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
	RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
	RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
	RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

	if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK) {
		Error_Handler();
	}
	PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USB;
	PeriphClkInit.UsbClockSelection = RCC_USBCLKSOURCE_PLL_DIV1_5;
	if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK) {
		Error_Handler();
	}
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
 * @brief  This function is executed in case of error occurrence.
 * @retval None
 */
void Error_Handler(void) {
	/* USER CODE BEGIN Error_Handler_Debug */
	/* User can add his own implementation to report the HAL error return state */
	__disable_irq();
	while (1) {
	}
	/* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
