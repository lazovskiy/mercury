#include <string>
#include <iostream>

class mercuryException {
	private:
		std::string error_message;

		std::string dumpBuffer(uint8_t *buf, uint8_t len);
        public:
		mercuryException(const char *function, std::string err);
                mercuryException(const char *function, uint8_t *request, uint8_t request_len,  uint8_t *response, uint8_t response_len, std::string err);

		friend std::ostream &operator<<(std::ostream &output, const mercuryException &m);
};
