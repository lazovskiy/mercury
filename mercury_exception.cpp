#include "mercury_exception.hpp"

mercuryException::mercuryException(const char *function, std::string err)
{
	error_message = std::string(function) + "(): " + err;
}

mercuryException::mercuryException(const char *function, uint8_t *request, uint8_t request_len,  uint8_t *response, uint8_t response_len, std::string err)
{
	error_message = std::string(function) + "(): " + err + "\n";
	error_message += "REQUEST(" + std::to_string(request_len) + "):\n";
	error_message += dumpBuffer(request, request_len) + "\n";

	error_message += "RESPONSE(" + std::to_string(response_len) + "):\n";
	error_message += dumpBuffer(response, response_len);
}

std::string mercuryException::dumpBuffer(uint8_t *buf, uint8_t len)
{
        const char *chars = "0123456789ABCDEF";
        uint8_t c, i;

	std::string ret;

	if(!buf || !len) {
		return "";
	}

	ret.reserve(len * 2 + len - 1);

	for(i = 0; i < len; i++) {
                c = buf[i];
                ret.push_back(chars[c >> 4]);
                ret.push_back(chars[c & 15]);

                if (i < len) {
                        ret.push_back(' ');
                }
	}

	return ret;
}

std::ostream &operator<<(std::ostream &output, const mercuryException &m)
{
	output << m.error_message;
	return output;
}
