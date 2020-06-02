#include <iostream>
#include <errno.h>
#include <unistd.h>
#include <getopt.h>
#include <iomanip>
#include <linux/spi/spidev.h>
#include <cstring>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <algorithm>
#include <wiringPi.h>
#include <ctime>

using namespace std;

int fd; //spi file descriptor
static const int chips = 1;
static int CS = 0; //Chip Select
uint8_t bits = 8; //bits per word
static __u32 speed = 50000000; //transmit speed in Hz
uint32_t mode = 0; //modes?
bool verbose = false;
bool reading = true;
static uint8_t buffer[chips];
static uint8_t recbuffer[chips];

static void pabort(const char *s) {
    perror(s);
    abort();
}


void arginit(int argc, char **argv) {
    int bit = 0;
    int value = -1;
    char val = ' ';
    while (true) {
        static const struct option lopts[] = {
                {"value",    1, 0, 'V'},
                {"hexvalue", 1, 0, 'H'},
                {"verbose",  1, 0, 'v'},
                {"speed",    1, 0, 's'},
                {"clear",    0, 0, 'c'},
                {"fill",     0, 0, 'f'},
                {NULL,       0, 0, 0},
        };
        int c = getopt_long(argc, argv, "V:s:H:vfc", lopts, NULL);
        if (c == -1)
            break;
        switch (c) {
            case 'v':
                verbose = true;
                break;
            case 's':
                speed = strtol(optarg, nullptr,10);
                break;
            case 'c':
                if (verbose)cout << "Clearing" << endl;
                reading = false;
                fill(begin(buffer), end(buffer), '\x00');
                if (verbose)cout << "Cleared" << endl;
                break;
            case 'f':
                if (verbose)cout << "Filling" << endl;
                reading = false;
                fill(begin(buffer), end(buffer), '\xff');
                if (verbose)cout << "Filled" << endl;
                break;
            case 'V':
                value = strtol(optarg, nullptr,10);
                reading = false;
                if (verbose) cout << bit << ":" << value << endl;
                switch (value) {
                    case 1:
                    case 0:
                        buffer[bit] = value;
                        break;
                    case -1:
                        break;
                    default:
                        cout
                                << "Value outside of bounds. Not 0 or 1.  That Doesn't make sense! Inverting existing value."
                                << endl;
                        buffer[bit] = !buffer[bit];
                        break;
                }
                bit++;
                break;
            case 'H':
                reading = false;
                if (strlen(optarg) > chips * 2) pabort("More bits than chips specified!");
                if (strlen(optarg) % 2 == 1) pabort("incomplete hex data!");
                for (int j = 0; j < strlen(optarg) / 2; j++) {
                    buffer[bit] = 0;
                    for (int i = 0; i < 2; i++) {
                        val = tolower(optarg[i]);
                        //index 0 accesses the top 4 bit. 1-i ensures, that the top 4 bit stay the top 4 bit.
                        //input order. smh. Not useful for hex!
                        if (val > '9' && val <= 'f') buffer[bit] += (val - 'a' + 10) << (1 - i) * 4;
                        if (val >= '0' && val <= '9') buffer[bit] += (val - '0') << (1 - i) * 4;
                    }
                    bit++;
                }
                break;
            default:
                cout << "wrong usage" << endl;
                break;
        }
    }
    if (verbose) {
        for (unsigned int var:buffer) {
            printf("%#0x\n", var);
        }
    }
}

void spiinit() {
    // Configure the interface.
    char spiDev[32];
    snprintf(spiDev, 31, "/dev/spidev0.%d", CS);
    fd = open(spiDev, O_RDWR);
    if (fd < 0) pabort("can't open spi device");
    mode = 0;
    mode |= SPI_CS_HIGH;

    if (ioctl(fd, SPI_IOC_WR_MODE32, &mode) < 0)
        pabort("can't set spi mode ");

    if (ioctl(fd, SPI_IOC_RD_MODE32, &mode) < 0)
        pabort("can't get spi mode");

    /*
	 * bits per word
	 */
    if (ioctl(fd, SPI_IOC_WR_BITS_PER_WORD, &bits) < 0)
        pabort("can't set bits per word");

    if (ioctl(fd, SPI_IOC_RD_BITS_PER_WORD, &bits) < 0)
        pabort("can't get bits per word");

    /*
	 * max speed hz
	 */
    if (ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed) < 0)
        pabort("can't set max speed hz");

    if (ioctl(fd, SPI_IOC_RD_MAX_SPEED_HZ, &speed) < 0)
        pabort("can't get max speed hz");

    if (verbose) printf("spi mode: 0x%x\n", mode);
    if (verbose) printf("bits per word: %d\n", bits);
    if (verbose) printf("max speed: %d Hz (%d KHz) (%d MHz)\n", speed, speed / 1000, speed / 1000000);
    if (verbose) printf("Init result: %d\n", fd);

}

void spimsg(uint8_t *tx = buffer, uint8_t *rx = recbuffer, uint size = chips) {
    struct spi_ioc_transfer tr = {
            .tx_buf = (unsigned long) tx,
            .rx_buf = (unsigned long) rx,
            .len = size,
            .speed_hz = speed,
            .delay_usecs = 0,
            .bits_per_word = bits,
    };

    if (ioctl(fd, SPI_IOC_MESSAGE(1), &tr) < 0)
        pabort("can't send spi message");
}

int main(int argc, char **argv) {

    //these are WiringPi pin numbers. I'm not going to implement my own GPIO system!
    const int CE0 = 24;
    const int CE1 = 26;
    const int SCLK = 23;

//    Feel free to replace with another library, but this is rn the easiest solution
    wiringPiSetupPhys();

    arginit(argc, argv);

    printf("Initializing\n");

    spiinit();

    if(reading){
        struct timespec sleep={};
        struct timespec remaining={};
        sleep.tv_sec=0;
        sleep.tv_nsec=50;

        //load inputs, load to registers, hold, read

        //parallel load
        digitalWrite(CE0,true);
        digitalWrite(CE1,true);
        nanosleep(&sleep,&remaining);//wait for the chip
        //Pulse clock to actually load
        digitalWrite(SCLK,false);
        digitalWrite(SCLK,true);
        nanosleep(&sleep,&remaining);//wait for the chip
        digitalWrite(SCLK,false);

        //parallel load
        digitalWrite(CE0,true);
        digitalWrite(CE1,true);
        nanosleep(&sleep,&remaining);//wait for the chip
        //Pulse clock to actually load
        digitalWrite(SCLK,false);
        digitalWrite(SCLK,true);
        nanosleep(&sleep,&remaining);//wait for the chip
        digitalWrite(SCLK,false);


        //reset load state to hold and disable input to prevent a continuous override of data
        digitalWrite(CE0,false);
        digitalWrite(CE1,false);
        nanosleep(&sleep,&remaining);//wait for the chip

        //Pulse Clock to propagate changes
        digitalWrite(SCLK,false);
        digitalWrite(SCLK,true);
        nanosleep(&sleep,&remaining);//wait for the chip
        digitalWrite(SCLK,false);
    }
    //now write with spi
    spimsg();

    if(reading){
        digitalWrite(CE0,false);
        digitalWrite(CE1,false);
    }

    //print data, if we actually read stuff
    printf("Received following data:\n");
    for (uint8_t val:recbuffer) {
        printf("%x ", val);
    }

    close(fd);
}