/**
 *
 * Copyright (c) 2026 Analog Devices, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */


/* Include global variables and configurations for the SoC.
*/
#include <stdio.h>
#include "common_data.h"

#include "lwip_socket.h"
#include "tcp_socket.h"
#include "no_os_error.h"
#include "mxc_delay.h"
#include "adin1110.h"
#include "max31855.h"
#include "network_interface.h"

#define SOCK_BUFF_MAX_LEN  			100
#define MIN_TELEMETRY_INTERVAL_US	100000 /* 100 ms */
#define MAX_OVERSAMPLE_LEN 			10 /* Max Temperature Conversion Time: 100 ms */
#define OVERSAMPLE_LEN     			5


typedef enum
{
	IDX_TC,
	IDX_AMB,
	IDX_MAX
} TempTypeIdx;


uint32_t telemetry_interval_us = 1000000;
volatile bool telemetryDue;
volatile uint32_t oversample_num;
volatile uint32_t oversample_idx;


void process_command(char *cmd);
uint32_t findMedianIndex(int32_t *data, uint32_t n);


void telemetryIntervalCb(int result)
{
	if (result != E_NO_ERROR)
	{
		telemetryDue = true;
	}
}

int main(void) 
{
  	bool connected = false;
	struct tcp_socket_desc *server_socket;
	struct tcp_socket_desc *client_socket;
	struct lwip_network_desc *lwip_desc;
	struct tcp_socket_init_param tcp_ip = {
		.max_buff_size = 0
	};

	uint8_t read_byte;
	uint8_t adin1110_mac_address[6] = {0x00, 0x18, 0x80, 0x03, 0x25, 0x60};
	struct no_os_uart_desc *uart_desc;

	char in_buff[SOCK_BUFF_MAX_LEN];
	char out_buff[SOCK_BUFF_MAX_LEN];
	uint16_t in_idx;
	uint16_t len;
	int ret;

	struct max31855_dev *thermo_dev;
	struct max31855_decimal temp[OVERSAMPLE_LEN][IDX_MAX];
	int32_t scaled_temp[OVERSAMPLE_LEN];
	uint32_t i;

	ret = no_os_uart_init(&uart_desc, &uart_ip);
	if (ret)
		return ret;

	no_os_uart_stdio(uart_desc);
	printf("UART stdio initialized\n");

	ret = max31855_init(&thermo_dev, &max31855_ip);
	if (ret != 0)
	{
		return ret;
	}

	memcpy(adin1110_ip.mac_address, adin1110_mac_address, NETIF_MAX_HWADDR_LEN);
	memcpy(lwip_ip.hwaddr, adin1110_mac_address, NETIF_MAX_HWADDR_LEN);

	ret = no_os_lwip_init(&lwip_desc, &lwip_ip);
	if (ret)
	{
		printf("LWIP initialization failed!\n");
		return ret;
	}

	tcp_ip.net = &lwip_desc->no_os_net;
	printf("LWIP initialized\n");

	ret = socket_init(&server_socket, &tcp_ip);
	if (ret)
	{
		printf("LWIP initialization failed!\n");
		return ret;
	}
	printf("Socket initialized\n");

	ret = socket_bind(server_socket, 10000);
	if (ret)
	{
		printf("Socket bind failed!\n");
		return ret;
	}
	printf("Socket bound\n");

	ret = socket_listen(server_socket, MAX_BACKLOG);
	if (ret)
	{
		printf("Socket listen failed!\n");
		return ret;
	}
	printf("Socket listening\n");

	while (1) 
	{
		no_os_lwip_step(server_socket->net->net, NULL);

		if (!connected)
		{
			ret = socket_accept(server_socket, &client_socket);
			if (ret == 0) 
			{
				connected = true;
				printf("Socket connected\n");
				telemetryDue = true;
				oversample_num = 1; // oversampling disabled, only one oversample per sample
				oversample_idx = 0;
				in_idx = 0;
			}
			else if (ret != -EAGAIN)
			{
				printf("Socket accept error: %d\n", ret);
				return ret;
			}
		}
		else 
		{
			if (telemetryDue == true)
			{
				if (oversample_idx < (oversample_num - 1))
				{
					MXC_DelayAsync(MIN_TELEMETRY_INTERVAL_US, telemetryIntervalCb);
				}
				else // this is the last oversample
				{
					MXC_DelayAsync(telemetry_interval_us - (MIN_TELEMETRY_INTERVAL_US * (oversample_num - 1)), telemetryIntervalCb);
				}
				telemetryDue = false;

				ret = max31855_read_temp(thermo_dev, &temp[oversample_idx][IDX_TC], &temp[oversample_idx][IDX_AMB]);
				if (ret != 0)
				{
					printf("Thermocouple read error\n");
				}
				else
				{
					oversample_idx++;
					if (oversample_idx == oversample_num)
					{
						oversample_idx = 0;

						// convert max31855 data type to fixed-point integer (will not change which oversample is the median)
						for (i = 0; i < oversample_num; i++)
						{
							scaled_temp[i] = (temp[i][IDX_TC].integer << 2) + temp[i][IDX_TC].decimal;
						}

						i = findMedianIndex(scaled_temp, oversample_num);

						len = sprintf(out_buff, "%.02d.%.02d, %.02d.%.02d\n", 
							temp[i][IDX_TC].integer, 
							(temp[i][IDX_TC].decimal * 100) >> 2, 
							temp[i][IDX_AMB].integer, 
							(temp[i][IDX_AMB].decimal * 100) >> 4);
						socket_send(client_socket, out_buff, len);
						printf("> %s", out_buff);
					}
				}
			}

			ret = socket_recv(client_socket, &read_byte, 1);
			if (ret > 0)
			{
				in_buff[in_idx] = read_byte;
				in_idx++;

				if ((read_byte == '\n') || (in_idx == (SOCK_BUFF_MAX_LEN - 1)))
				{
					in_buff[in_idx] = '\0';
					in_idx = 0;
					printf("< %s", in_buff);
					process_command(in_buff);
				}
			}
			else if (ret < 0 && ret != -EAGAIN)
			{
				printf("Socket receive error: %d, socket disconnected\n", ret);
				socket_remove(client_socket);
				connected = false;

				MXC_DelayAbort();
				telemetryDue = false; 
			}
		}
	}

	return 0;
}

void process_command(char *cmd)
{
	uint32_t arg1;
	int ret;

	if (strncmp(cmd, "INTERVAL", 8) == 0)
	{
		ret = sscanf(cmd + 8, " %d\n", &arg1);
		if (ret == 1)
		{
			if ((arg1 > 0) && (arg1 < 900))
			{
				telemetry_interval_us = arg1 * 1000000; 
				printf("OK\n");
			}
			else
			{
				printf("Invalid arg\n");
			}
		}
		else
		{
			printf("Invalid fmt\n");
		}
	}
	else if (strncmp(cmd, "FILTER", 6) == 0)
	{
		ret = sscanf(cmd + 6, " %d\n", &arg1);
		if (ret == 1)
		{
			if ((arg1 == 0) || (arg1 == 1))
			{
				oversample_num = (arg1 ? OVERSAMPLE_LEN : 1);
				if (oversample_idx >= oversample_num)
				{
					oversample_idx = oversample_num - 1;
				}
				printf("OK\n");
			}
			else
			{
				printf("Invalid arg\n");
			}
		}
		else
		{
			printf("Invalid fmt\n");
		}
	}
	else
	{
		printf("Invalid cmd\n");
	}
}

uint32_t findMedianIndex(int32_t *data, uint32_t n)
{
    uint32_t idx[MAX_OVERSAMPLE_LEN];
	uint32_t key;
	uint32_t i, j;

	if (data == NULL || n == 0 || n > MAX_OVERSAMPLE_LEN)
	{
		return -1;
	}

    for (i = 0; i < n; i++)
	{
        idx[i] = i;
	}

    /* Insertion sort (efficient for such small arrays) */
    for (i = 1; i < n; i++) 
	{
        key = idx[i];
        j = i;
        while (j > 0 && data[idx[j - 1]] > data[key]) 
		{
            idx[j] = idx[j - 1];
            j--;
        }
        idx[j] = key;
    }

	// Lazy, return index of "upper median" for even numbers
	return idx[n / 2];
}