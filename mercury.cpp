#include "mercury.hpp"

mercury::mercury(std::string p)
{
	port = p;

	portOpen();
	portConfigure();

	max_retries = 16;

	request(0, "\x01\x01\x01\x01\x01\x01\x01\x01", 8);
}

mercury::~mercury(void)
{
	channelClose();
}

void mercury::portOpen(void)
{
	int fd;
	std::string err;

	if ( (fd = open (port.c_str(), O_RDWR|O_NOCTTY|O_SYNC)) < 0) {
		err = "Unable to open " + port + " (" + std::to_string(errno) + ": " + strerror(errno) + ")";
		throw mercuryException(__FUNCTION__, err);
	}

	port_fd = fd;
}

void mercury::portConfigure(void)
{
	struct termios tty;
	std::string err;

	memset (&tty, 0, sizeof tty);

        if (tcgetattr(port_fd, &tty) != 0) {
		err = "Unable to get terminal attributes (" + std::to_string(errno) + ": " + strerror(errno) + ")";
		throw mercuryException(__FUNCTION__, err);
        }

	cfmakeraw(&tty);

	cfsetospeed (&tty, B9600);
	cfsetispeed (&tty, B9600);

	tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8; // 8-bit chars
	tty.c_iflag &= ~IGNBRK;                     // ignore break signal
	tty.c_cc[VMIN]  = 0;                        // read doesn't block
	tty.c_cc[VTIME] = 5;                        // 0.5 seconds read timeout

	tty.c_iflag &= ~(IXON | IXOFF | IXANY);     // shut off xon/xoff ctrl

	tty.c_cflag |= (CLOCAL | CREAD);            // ignore modem controls, enable reading
	tty.c_cflag &= ~(PARENB | PARODD);          // shut off parity
	tty.c_cflag |= 0;
	tty.c_cflag &= ~CSTOPB;
	tty.c_cflag &= ~CRTSCTS;

	if (tcsetattr(port_fd, TCSANOW, &tty) != 0) {
		err = "Unable to set terminal attributes (" + std::to_string(errno) + ": " + strerror(errno) + ")";
		throw mercuryException(__FUNCTION__, err);
        }
}

void mercury::channelOpen(void)
{
	request(0, "\x01\x01\x01\x01\x01\x01\x01\x01", 8);
}

void mercury::channelClose(void)
{
	request(0, "\x02", 1);
	close(port_fd);
}

response_t mercury::request(uint8_t device_addr, const char *cmd, size_t cmd_len)
{
	unsigned char request[32], response[32];
	uint8_t request_len = 0, response_len = 0;
	uint16_t request_crc = 0, response_crc = 0;

	bool done;
	uint8_t attempt = 0;
	std::string err;

	response_t ret;

	if(cmd_len > sizeof(request) - 3) {
		throw mercuryException(__FUNCTION__, "too long cmd");
	}

	request[request_len++] = device_addr;
	memcpy(&request[request_len], cmd, cmd_len);
	request_len += cmd_len;

	request_crc = mkCRC16(request, cmd_len + 1);
	request[request_len++] = ((uint8_t*)&request_crc)[1];
	request[request_len++] = ((uint8_t*)&request_crc)[0];

	do {
		if(attempt) {
			usleep(50000 * attempt);
		}

		attempt++;

		write(port_fd, request, request_len);
		response_len = read(port_fd, response, sizeof(response));

		/* Minimum response length is 4 bytes */
		if(response_len < 4) {
			err = "invalid response";
			continue;
		}

		/* Check response CRC */
		response_crc = response[response_len - 1];
		response_crc |= response[response_len - 2] << 8;

		if(!ignore_checksum && response_crc != mkCRC16(response, response_len - 2)) {
			err = "invalid response checksum";
			continue;
		}

		/* Device address in both request and response must be the same */
		if(response[0] != device_addr) {
			err = "invalid device address";
			continue;
		}

		/* For status response, check if it's OK */
		if(response_len == 4 && (response[1] & 0xF) != 0) {
			err = "device returned error";
			continue;
		}

		done = true;

	} while(!done && attempt < max_retries);

	if(!done) {
		throw mercuryException(__FUNCTION__, request, request_len, response, response_len, err);
	}

	ret.device_addr = device_addr;
	ret.crc = response_crc;
	memcpy(ret.data, &response[1], response_len - 3);
	ret.data_len = response_len - 3;

	return ret;	
}

std::vector<float> mercury::getParam(mercury_param_t param)
{
	uint8_t mask = 0x3F;
	uint8_t response_len = 12;
	uint16_t divisor = 100;
	const char *cmd;

	std::vector<float> ret;
	response_t r;
	float val;

	switch(param) {
		case MERCURY_VOLTAGE :
			cmd = "\x08\x16\x11";
			mask = 0xFF;
			response_len = 9;
			break;
		case MERCURY_CURRENT :
			cmd = "\x08\x16\x21";
			mask = 0xFF;
			divisor = 1000;
			response_len = 9;
			break;
		case MERCURY_POWER_REAL :
			cmd = "\x08\x16\x00";
			break;
		case MERCURY_POWER_REACTIVE :
			cmd = "\x08\x16\x04";
			break;
		case MERCURY_POWER_APPARENT :
			cmd = "\x08\x16\x08";
			break;
		case MERCURY_POWER_FACTOR :
			cmd = "\x08\x16\x30";
			divisor = 1000;
			break;

		default :
			break;
			
	}

        r = request(0, cmd, 3);

        if(r.data_len != response_len) {
                throw mercuryException(__FUNCTION__, "invalid response length. Expected " + std::to_string(response_len) + ", received " + std::to_string(r.data_len) + " bytes");
        }

        for(int offset = 0; offset < 4; offset++) {
                val = (float)mkInt32(r.data[offset * 3 + 1], r.data[offset * 3 + 2], r.data[offset * 3] & mask, 0) / divisor;
                ret.push_back(val);
        }

        return ret;

}

std::vector<float> mercury::getVoltages(void)
{
	return getParam(MERCURY_VOLTAGE);
}

std::vector<float> mercury::getCurrents(void)
{
	return getParam(MERCURY_CURRENT);
}

std::vector<float> mercury::getPowersReal(void)
{
	return getParam(MERCURY_POWER_REAL);
}

std::vector<float> mercury::getPowersReactive(void)
{
	return getParam(MERCURY_POWER_REACTIVE);
}

std::vector<float> mercury::getPowersApparent(void)
{
	return getParam(MERCURY_POWER_APPARENT);
}

std::vector<float> mercury::getPFs(void)
{
	return getParam(MERCURY_POWER_FACTOR);
}

std::vector<uint32_t> mercury::getPowerRegistration(void)
{
	std::vector<uint32_t> ret;
        response_t r;
	uint32_t val;

	r = request(0, "\x05\x00\x00", 3);

	if(r.data_len != 16) {
		throw mercuryException(__FUNCTION__, "invalid response length. Expected 16, received " + std::to_string(r.data_len) + " bytes");
	}

        for(int offset = 0; offset < 4; offset++) {
                val = mkInt32(r.data[offset * 4 + 2], r.data[offset * 4 + 3], r.data[offset * 4 + 0], r.data[offset * 4 + 1]);

		if(val == 0xFFFFFFFF) {
			val = 0;
		}

		ret.push_back(val);
        }

	return ret;
}

uint32_t mercury::mkInt32(uint8_t b1, uint8_t b2, uint8_t b3, uint8_t b4)
{
	uint8_t arr[] = {b1, b2, b3, b4};
	return *(uint32_t*)arr;
}

uint16_t mercury::mkCRC16(const uint8_t *buf, size_t len)
{
	static const uint16_t crc16_table[] = {

		0X0000, 0XC0C1, 0XC181, 0X0140, 0XC301, 0X03C0, 0X0280, 0XC241,
		0XC601, 0X06C0, 0X0780, 0XC741, 0X0500, 0XC5C1, 0XC481, 0X0440,
		0XCC01, 0X0CC0, 0X0D80, 0XCD41, 0X0F00, 0XCFC1, 0XCE81, 0X0E40,
		0X0A00, 0XCAC1, 0XCB81, 0X0B40, 0XC901, 0X09C0, 0X0880, 0XC841,
		0XD801, 0X18C0, 0X1980, 0XD941, 0X1B00, 0XDBC1, 0XDA81, 0X1A40,
		0X1E00, 0XDEC1, 0XDF81, 0X1F40, 0XDD01, 0X1DC0, 0X1C80, 0XDC41,
		0X1400, 0XD4C1, 0XD581, 0X1540, 0XD701, 0X17C0, 0X1680, 0XD641,
		0XD201, 0X12C0, 0X1380, 0XD341, 0X1100, 0XD1C1, 0XD081, 0X1040,
		0XF001, 0X30C0, 0X3180, 0XF141, 0X3300, 0XF3C1, 0XF281, 0X3240,
		0X3600, 0XF6C1, 0XF781, 0X3740, 0XF501, 0X35C0, 0X3480, 0XF441,
		0X3C00, 0XFCC1, 0XFD81, 0X3D40, 0XFF01, 0X3FC0, 0X3E80, 0XFE41,
		0XFA01, 0X3AC0, 0X3B80, 0XFB41, 0X3900, 0XF9C1, 0XF881, 0X3840,
		0X2800, 0XE8C1, 0XE981, 0X2940, 0XEB01, 0X2BC0, 0X2A80, 0XEA41,
		0XEE01, 0X2EC0, 0X2F80, 0XEF41, 0X2D00, 0XEDC1, 0XEC81, 0X2C40,
		0XE401, 0X24C0, 0X2580, 0XE541, 0X2700, 0XE7C1, 0XE681, 0X2640,
		0X2200, 0XE2C1, 0XE381, 0X2340, 0XE101, 0X21C0, 0X2080, 0XE041,
		0XA001, 0X60C0, 0X6180, 0XA141, 0X6300, 0XA3C1, 0XA281, 0X6240,
		0X6600, 0XA6C1, 0XA781, 0X6740, 0XA501, 0X65C0, 0X6480, 0XA441,
		0X6C00, 0XACC1, 0XAD81, 0X6D40, 0XAF01, 0X6FC0, 0X6E80, 0XAE41,
		0XAA01, 0X6AC0, 0X6B80, 0XAB41, 0X6900, 0XA9C1, 0XA881, 0X6840,
		0X7800, 0XB8C1, 0XB981, 0X7940, 0XBB01, 0X7BC0, 0X7A80, 0XBA41,
		0XBE01, 0X7EC0, 0X7F80, 0XBF41, 0X7D00, 0XBDC1, 0XBC81, 0X7C40,
		0XB401, 0X74C0, 0X7580, 0XB541, 0X7700, 0XB7C1, 0XB681, 0X7640,
		0X7200, 0XB2C1, 0XB381, 0X7340, 0XB101, 0X71C0, 0X7080, 0XB041,
		0X5000, 0X90C1, 0X9181, 0X5140, 0X9301, 0X53C0, 0X5280, 0X9241,
		0X9601, 0X56C0, 0X5780, 0X9741, 0X5500, 0X95C1, 0X9481, 0X5440,
		0X9C01, 0X5CC0, 0X5D80, 0X9D41, 0X5F00, 0X9FC1, 0X9E81, 0X5E40,
		0X5A00, 0X9AC1, 0X9B81, 0X5B40, 0X9901, 0X59C0, 0X5880, 0X9841,
		0X8801, 0X48C0, 0X4980, 0X8941, 0X4B00, 0X8BC1, 0X8A81, 0X4A40,
		0X4E00, 0X8EC1, 0X8F81, 0X4F40, 0X8D01, 0X4DC0, 0X4C80, 0X8C41,
		0X4400, 0X84C1, 0X8581, 0X4540, 0X8701, 0X47C0, 0X4680, 0X8641,
		0X8201, 0X42C0, 0X4380, 0X8341, 0X4100, 0X81C1, 0X8081, 0X4040
	};

	uint16_t ret = 0xFFFF;

	while (len--) {
		ret = (ret >> 8) ^ crc16_table[(ret ^ *buf++) & 0xFF];
	}

	return ((ret & 0xFF) << 8) | ((ret & 0xFF00) >> 8);
}
