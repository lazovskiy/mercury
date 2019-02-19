
extern "C" {
	#include <sys/types.h>
	#include <sys/stat.h>
	#include <termios.h>
	#include <unistd.h>
	#include <string.h>
	#include <fcntl.h>
	#include <stdio.h>
	#include <stdint.h>
	#include <errno.h>
}

#include <string>
#include <vector>
#include <iostream>

#include "mercury_exception.hpp"

typedef struct {
	uint8_t device_addr;
	uint16_t crc;
	uint8_t data[32];
	uint8_t data_len;
} response_t;

typedef enum {
	MERCURY_VOLTAGE,
	MERCURY_CURRENT,
	MERCURY_POWER_REAL,
	MERCURY_POWER_REACTIVE,
	MERCURY_POWER_APPARENT,
	MERCURY_POWER_FACTOR
} mercury_param_t;

class mercury {
	private:
		std::string port;
		int port_fd;
		uint8_t max_retries;

		void portOpen(void);
		void portConfigure(void);
		void channelOpen(void);
		void channelClose(void);

		response_t request(uint8_t device_addr, const char *cmd, size_t cmd_len);
		uint32_t mkInt32(uint8_t b4, uint8_t b3, uint8_t b2, uint8_t b1);
		uint16_t mkCRC16(const uint8_t *buf, size_t len);

		std::vector<float> getParam(mercury_param_t param);

	public:
		bool ignore_checksum = false;

		mercury(std::string p);
		~mercury(void);
		std::vector<float> getVoltages(void);
		std::vector<float> getCurrents(void);
		std::vector<float> getPowersReal(void);
		std::vector<float> getPowersReactive(void);
		std::vector<float> getPowersApparent(void);
		std::vector<float> getPFs(void);
		std::vector<uint32_t> getPowerRegistration(void);

};

