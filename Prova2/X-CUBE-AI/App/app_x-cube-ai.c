
#ifdef __cplusplus
 extern "C" {
#endif
/**
  ******************************************************************************
  * @file           : app_x-cube-ai.c
  * @brief          : AI program body
  ******************************************************************************
  * This notice applies to any and all portions of this file
  * that are not between comment pairs USER CODE BEGIN and
  * USER CODE END. Other portions of this file, whether
  * inserted by the user or by software development tools
  * are owned by their respective copyright owners.
  *
  * Copyright (c) 2018 STMicroelectronics International N.V.
  * All rights reserved.
  *
  * Redistribution and use in source and binary forms, with or without
  * modification, are permitted, provided that the following conditions are met:
  *
  * 1. Redistribution of source code must retain the above copyright notice,
  *    this list of conditions and the following disclaimer.
  * 2. Redistributions in binary form must reproduce the above copyright notice,
  *    this list of conditions and the following disclaimer in the documentation
  *    and/or other materials provided with the distribution.
  * 3. Neither the name of STMicroelectronics nor the names of other
  *    contributors to this software may be used to endorse or promote products
  *    derived from this software without specific written permission.
  * 4. This software, including modifications and/or derivative works of this
  *    software, must execute solely and exclusively on microcontroller or
  *    microprocessor devices manufactured by or for STMicroelectronics.
  * 5. Redistribution and use of this software other than as permitted under
  *    this license is void and will automatically terminate your rights under
  *    this license.
  *
  * THIS SOFTWARE IS PROVIDED BY STMICROELECTRONICS AND CONTRIBUTORS "AS IS"
  * AND ANY EXPRESS, IMPLIED OR STATUTORY WARRANTIES, INCLUDING, BUT NOT
  * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
  * PARTICULAR PURPOSE AND NON-INFRINGEMENT OF THIRD PARTY INTELLECTUAL PROPERTY
  * RIGHTS ARE DISCLAIMED TO THE FULLEST EXTENT PERMITTED BY LAW. IN NO EVENT
  * SHALL STMICROELECTRONICS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
  * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
  * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
  * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
  * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
  * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
  * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  *
  ******************************************************************************
  */
 /*
  * Description
  *   v1.0 - Minimum template to show how to use the Embedded Client API
  *          model. Only one input and one output is supported. All
  *          memory resources are allocated statically (AI_NETWORK_XX, defines
  *          are used).
  *          Re-target of the printf function is out-of-scope.
  *
  *   For more information, see the embeded documentation:
  *
  *       [1] %X_CUBE_AI_DIR%/Documentation/index.html
  *
  *   X_CUBE_AI_DIR indicates the location where the X-CUBE-AI pack is installed
  *   typical : C:\Users\<user_name>\STM32Cube\Repository\STMicroelectronics\X-CUBE-AI\6.0.0
  */
/* Includes ------------------------------------------------------------------*/
/* System headers */
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <inttypes.h>
#include <string.h>

#include "app_x-cube-ai.h"
#include "main.h"
#include "ai_datatypes_defines.h"

/* USER CODE BEGIN includes */
//importazione delle librerie necessarie
#include "stdlib.h"
#include <math.h>
//definizione delle variabili esterne
extern UART_HandleTypeDef huart1;
extern float values1[];
extern TIM_HandleTypeDef htim3;

//definizione e inizializzazione delle variabili
int v;
int k=0;
int val[1]={};
uint32_t time;
uint8_t buffertx[50]={0};
uint8_t buffertx1[100]={0};

//definizione della funzione che restituisce l'indice del valore massimo in un array
int max(float a[], int dim){
	float max=0;
	int index;
	for (int i=0; i<dim; i++) {
		if (max<a[i])
		{
			max=a[i];
			index=i;
		}
	}
	return index;
}


//definizione della funzione che va a riempire un array di dimensione pari a k=30
void riempi_array(int x){
	if(k<1){
		val[k]=x;
		k++;
	}else{
		k=0;
		val[k]=x;
		k++;
	}
}

/* USER CODE END includes */
/* Global AI objects */
static ai_handle network = AI_HANDLE_NULL;
static ai_network_report network_info;

/* Global c-array to handle the activations buffer */
AI_ALIGNED(4)
static ai_u8 activations[AI_NETWORK_DATA_ACTIVATIONS_SIZE];

/*  In the case where "--allocate-inputs" option is used, memory buffer can be
 *  used from the activations buffer. This is not mandatory.
 */
#if !defined(AI_NETWORK_INPUTS_IN_ACTIVATIONS)
/* Allocate data payload for input tensor */
AI_ALIGNED(4)
static ai_u8 in_data_s[AI_NETWORK_IN_1_SIZE_BYTES];
#endif

/*  In the case where "--allocate-outputs" option is used, memory buffer can be
 *  used from the activations buffer. This is no mandatory.
 */
#if !defined(AI_NETWORK_OUTPUTS_IN_ACTIVATIONS)
/* Allocate data payload for the output tensor */
AI_ALIGNED(4)
static ai_u8 out_data_s[AI_NETWORK_OUT_1_SIZE_BYTES];
#endif

static void ai_log_err(const ai_error err, const char *fct)
{
  /* USER CODE BEGIN log */
	if (fct)
		printf("TEMPLATE - Error (%s) - type=0x%02x code=0x%02x\r\n", fct,
				err.type, err.code);
	else
		printf("TEMPLATE - Error - type=0x%02x code=0x%02x\r\n", err.type, err.code);

	do {} while (1);
  /* USER CODE END log */
}

static int ai_boostrap(ai_handle w_addr, ai_handle act_addr)
{
  ai_error err;

  /* 1 - Create an instance of the model */
  err = ai_network_create(&network, AI_NETWORK_DATA_CONFIG);
  if (err.type != AI_ERROR_NONE) {
    ai_log_err(err, "ai_network_create");
    return -1;
  }

  /* 2 - Initialize the instance */
  const ai_network_params params = {
      AI_NETWORK_DATA_WEIGHTS(w_addr),
      AI_NETWORK_DATA_ACTIVATIONS(act_addr) };

  if (!ai_network_init(network, &params)) {
      err = ai_network_get_error(network);
      ai_log_err(err, "ai_network_init");
      return -1;
    }

  /* 3 - Retrieve the network info of the created instance */
  if (!ai_network_get_info(network, &network_info)) {
    err = ai_network_get_error(network);
    ai_log_err(err, "ai_network_get_error");
    ai_network_destroy(network);
    network = AI_HANDLE_NULL;
    return -3;
  }

  return 0;
}

static int ai_run(void *data_in, void *data_out)
{
  ai_i32 batch;

  ai_buffer *ai_input = network_info.inputs;
  ai_buffer *ai_output = network_info.outputs;

  ai_input[0].data = AI_HANDLE_PTR(data_in);
  ai_output[0].data = AI_HANDLE_PTR(data_out);

  batch = ai_network_run(network, ai_input, ai_output);
  if (batch != 1) {
    ai_log_err(ai_network_get_error(network),
        "ai_network_run");
    return -1;
  }

  return 0;
}

/* USER CODE BEGIN 2 */
//definizone della funzione di preprocessing
int acquire_and_process_data(void * data)
{
	ai_handle *in_data=AI_HANDLE_PTR(data);

	for(int i=0; i<120; i++){
		((ai_float *)in_data)[i]= values1[i];
	}
	return 0;
}

//definizione della funzione di postprocessing
int post_process(void * out_data, void * in_data)
{
	float f[4];

	char a[] = { ((char *)out_data)[0], ((char *)out_data)[1], ((char *)out_data)[2], ((char *)out_data)[3] };
	char b[] = { ((char *)out_data)[4], ((char *)out_data)[5], ((char *)out_data)[6], ((char *)out_data)[7] };
	char c[] = { ((char *)out_data)[8], ((char *)out_data)[9], ((char *)out_data)[10], ((char *)out_data)[11] };
	char d[]= { ((char *)out_data)[12], ((char *)out_data)[13], ((char *)out_data)[14], ((char *)out_data)[15] };
	memcpy(&f[0], &a, sizeof(f[0]));
	memcpy(&f[1], &b, sizeof(f[1]));
	memcpy(&f[2], &c, sizeof(f[2]));
	memcpy(&f[3], &d, sizeof(f[3]));

	int m=0;
	float media=0;
	int sum=0;
	int cont=0;
	v=max(f, sizeof(f));
	riempi_array(v);
	if(k==1){
		for(int h=0; h<1; h++){
			if(val[h]!=0){
				sum=sum+val[h];
				cont++;
			}
		}
		media=(float)sum/cont;
		m=roundf(media);
		if(m==0){
			sprintf(buffertx, "Veicolo fermo\n\r");
			HAL_GPIO_WritePin(ARD_D9_GPIO_Port, ARD_D9_Pin, 0);
			HAL_GPIO_WritePin(ARD_D8_GPIO_Port, ARD_D8_Pin, 0);
			HAL_GPIO_WritePin(ARD_D10_GPIO_Port, ARD_D10_Pin, 0);
		}
		if(m==1){
			sprintf(buffertx, "Guida sicura\n\r");
			HAL_GPIO_WritePin(ARD_D8_GPIO_Port, ARD_D8_Pin, 0);
			HAL_GPIO_WritePin(ARD_D9_GPIO_Port, ARD_D9_Pin, 1);
			HAL_GPIO_WritePin(ARD_D10_GPIO_Port, ARD_D10_Pin, 0);
		}
		if(m==2){
			sprintf(buffertx, "Guida normale\n\r");
			HAL_GPIO_WritePin(ARD_D8_GPIO_Port, ARD_D8_Pin, 1);
			HAL_GPIO_WritePin(ARD_D9_GPIO_Port, ARD_D9_Pin, 0);
			HAL_GPIO_WritePin(ARD_D10_GPIO_Port, ARD_D10_Pin, 0);
		}
		if(m==3){
			sprintf(buffertx, "Guida non adeguata\n\r");
			HAL_GPIO_WritePin(ARD_D8_GPIO_Port, ARD_D8_Pin, 0);
			HAL_GPIO_WritePin(ARD_D9_GPIO_Port, ARD_D9_Pin, 0);
			HAL_GPIO_WritePin(ARD_D10_GPIO_Port, ARD_D10_Pin, 1);
		}
		HAL_UART_Transmit(&huart1, buffertx, strlen(buffertx), 100);


	}
	return -1;
}
/* USER CODE END 2 */

/*************************************************************************
  *
  */
void MX_X_CUBE_AI_Init(void)
{
    /* USER CODE BEGIN 5 */
	printf("\r\nTEMPLATE - initialization\r\n");

	ai_boostrap(ai_network_data_weights_get(), activations);
    /* USER CODE END 5 */
}

void MX_X_CUBE_AI_Process(void)
{
    /* USER CODE BEGIN 6 */


	int res = -1;
	ai_u8 *in_data = NULL;
	ai_u8 *out_data = NULL;


	printf("TEMPLATE - run - main loop\r\n");

	if (network) {

		if ((network_info.n_inputs != 1) || (network_info.n_outputs != 1)) {
			ai_error err = {AI_ERROR_INVALID_PARAM, AI_ERROR_CODE_OUT_OF_RANGE};
			ai_log_err(err, "template code should be updated\r\n to support a model with multiple IO");
			return;
		}

		/* 1 - Set the I/O data buffer */

#if AI_NETWORK_INPUTS_IN_ACTIVATIONS
		in_data = network_info.inputs[0].data;
#else
		in_data = in_data_s;
#endif

#if AI_NETWORK_OUTPUTS_IN_ACTIVATIONS
		out_data = network_info.outputs[0].data;
#else
		out_data = out_data_s;
#endif

		if ((!in_data) || (!out_data)) {
			printf("TEMPLATE - I/O buffers are invalid\r\n");
			return;
		}

		/* 2 - main loop */
		do {
			/* 1 - acquire and pre-process input data */
			res = acquire_and_process_data(in_data);
			/* 2 - process the data - call inference engine */

			if (res == 0){
				//				__HAL_TIM_SET_COUNTER(&htim3, 0);
				//				sprintf(buffertx1, "Start\n\r");
				//				HAL_UART_Transmit(&huart1, buffertx1, sizeof(buffertx1), 1000);
				//				HAL_GPIO_TogglePin(ARD_D8_GPIO_Port, ARD_D8_Pin);
				res = ai_run(in_data, out_data);
				//				HAL_GPIO_TogglePin(ARD_D8_GPIO_Port, ARD_D8_Pin);
				//				time=__HAL_TIM_GET_COUNTER(&htim3);
				//				float M=time*0.1;
				//				sprintf(buffertx1, "Time value: %f ms\n\r", M);
				//				HAL_UART_Transmit(&huart1, buffertx1, sizeof(buffertx1), 1000);
			}
			/* 3- post-process the predictions */
			if (res == 0)
				res = post_process(out_data, in_data);

		} while (res==0);
	}

	if (res==1) {
		ai_error err = {AI_ERROR_INVALID_STATE, AI_ERROR_CODE_NETWORK};
		ai_log_err(err, "Process has FAILED");
	}
    /* USER CODE END 6 */
}
#ifdef __cplusplus
}
#endif
