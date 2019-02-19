
#include "main.hpp"

int
main(int argc, char **argv)
{
	int output_rrd = 0;
	int output_collectd = 0;
	char *opt_port = NULL;

	static struct option long_options[] = {
		{"rrd", no_argument, &output_rrd, 1},
		{"collectd", no_argument, &output_collectd, 1},
		{"port", required_argument, 0, 'p'},
		{0, 0, 0, 0}
	};

	std::vector<float> voltage, current, power_real, power_reactive, power_apparent, power_factor;
	std::vector<uint32_t> power_total;

	mercury *m;

	int c, option_index = 0;
     
	while(true) {
		c = getopt_long(argc, argv, "p:", long_options, &option_index);

		switch(c) {
			case 'p' :
				opt_port = optarg;
				break;

			case '?' :
				return 1;
		}

     		if (c == -1) {
			break;
		}
	}

	std::string port = "/dev/ttyACM0";

	if (opt_port) {
		port = opt_port;
	}

	setvbuf(stdout, NULL, _IONBF, 0);

	while (true) {

		try {
			m = new mercury(port);
			voltage = m->getVoltages();
			current = m->getCurrents();
			power_real     = m->getPowersReal();
			power_reactive = m->getPowersReactive();
			power_apparent = m->getPowersApparent();
			power_factor   = m->getPFs();
			if (!output_rrd) {
				power_total = m->getPowerRegistration();
			}
		} catch (mercuryException &e) {
			std::cerr << "mercury error: " << e << std::endl;
	
			if (!output_collectd) {
				return 1;
			}
		}

		if (output_rrd) {
			printf("voltage N:%.2f:%.2f:%.2f\n", voltage[0], voltage[1], voltage[2]);
			printf("current N:%.2f:%.2f:%.2f\n", current[0], current[1], current[2]);
			printf("power-real N:%.2f:%.2f:%.2f\n", power_real[1], power_real[2], power_real[3]);
			printf("power-reactive N:%.2f:%.2f:%.2f\n", power_reactive[1], power_reactive[2], power_reactive[3]);
			printf("power-apparent N:%.2f:%.2f:%.2f\n", power_apparent[1], power_apparent[2], power_apparent[3]);
			printf("power-factor N:%.3f:%.3f:%.3f\n", power_factor[1], power_factor[2], power_factor[3]);
			break;
		}

		if (output_collectd) {
			printf("PUTVAL moidom02/mercury/voltage interval=10 N:%.2f:%.2f:%.2f\n", voltage[0], voltage[1], voltage[2]);
			printf("PUTVAL moidom02/mercury/current interval=10 N:%.2f:%.2f:%.2f\n", current[0], current[1], current[2]);
			printf("PUTVAL moidom02/mercury/pf interval=10 N:%.3f:%.3f:%.3f\n", power_factor[1], power_factor[2], power_factor[3]);
			printf("PUTVAL moidom02/mercury/power-real interval=10 N:%.2f:%.2f:%.2f\n", power_real[1], power_real[2], power_real[3]);
			printf("PUTVAL moidom02/mercury/power-reactive interval=10 N:%.2f:%.2f:%.2f\n", power_reactive[1], power_reactive[2], power_reactive[3]);
			printf("PUTVAL moidom02/mercury/power-apparent interval=10 N:%.2f:%.2f:%.2f\n", power_apparent[1], power_apparent[2], power_apparent[3]);
			
			delete m;
			sleep(10);
			continue;
		}

		printf("                               A          B          C\n");
		printf("Voltage (V):          %10.2f %10.2f %10.2f\n", voltage[0], voltage[1], voltage[2]);
		printf("Current (A):          %10.2f %10.2f %10.2f\n", current[0], current[1], current[2]);
		printf("Power real (W):       %10.2f %10.2f %10.2f\n", power_real[1], power_real[2], power_real[3]);
		printf("Power apparent (VA):  %10.2f %10.2f %10.2f\n", power_apparent[1], power_apparent[2], power_apparent[3]);
		printf("Power reactive (VAr): %10.2f %10.2f %10.2f\n", power_reactive[1], power_reactive[2], power_reactive[3]);
		printf("Power factor:         %10.3f %10.3f %10.3f\n", power_factor[1], power_factor[2], power_factor[3]);
		printf("\n");
		printf("Total power consumption: %.2f VA\n", power_apparent[0]);
		printf("Total power factor: %.3f\n", power_factor[0]);
		printf("\n");
		printf("Active forward: %.2f kWh\n", (float)power_total[0] / 1000);
		printf("Active reverse: %.2f kWh\n", (float)power_total[1] / 1000);
		printf("Reactive forward: %.2f kWh\n", (float)power_total[2] / 1000);
		printf("Reactive reverse: %.2f kWh\n", (float)power_total[3] / 1000);
		break;
	}

	delete m;
	return 0;
}
