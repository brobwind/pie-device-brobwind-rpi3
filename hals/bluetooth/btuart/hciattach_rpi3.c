#include <stdio.h>
#include <errno.h>
#include <dirent.h>
#include <fcntl.h>
#include <string.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <poll.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/utsname.h>
#include <ctype.h>
#include <stdarg.h>
#include <poll.h>

#include <unistd.h>
#include <time.h>

#define LOG_TAG "BTUART"
#include <log/log.h>

#include "hciattach_rpi3.h"

#if 0
void ALOGE(const char *fmt, ...) {
	char buf[4096];
	va_list ap;

	va_start(ap, fmt);

	vsprintf(buf, fmt, ap);
	va_end(ap);

	printf("%s\n", buf);
}

void ALOGI(const char *fmt, ...) {
	char buf[4096];
	va_list ap;

	va_start(ap, fmt);

	vsprintf(buf, fmt, ap);
	va_end(ap);

	printf("%s\n", buf);
}
#endif


#define FLOW_CTL	0x0001
#define AMP_DEV		0x0002
#define ENABLE_PM	1
#define DISABLE_PM	0

struct uart_t {
	char *type;
	int  m_id;
	int  p_id;
	int  proto;
	int  init_speed;
	int  speed;
	int  flags;
	int  pm;
	char *bdaddr;
	int  (*init) (int fd, struct uart_t *u, struct termios *ti);
	int  (*post) (int fd, struct uart_t *u, struct termios *ti);
};

static int bcm43xx(int fd, struct uart_t *u, struct termios *ti);

struct uart_t uart[] = {
	/* Broadcom BCM43XX */
	{ "bcm43xx",    0x0000, 0x0000, HCI_UART_H4,   115200, 3000000,
				FLOW_CTL, DISABLE_PM, NULL, bcm43xx, NULL  },
};

static volatile sig_atomic_t __io_canceled = 0;

static void sig_hup(int sig)
{
	(void)sig;
}

static void sig_term(int sig)
{
	(void)sig;
	__io_canceled = 1;
}

static void sig_alarm(int sig)
{
	(void)sig;
	ALOGE("Initialization timed out.");
	exit(1);
}

int set_speed(int fd, struct termios *ti, int speed)
{
	ALOGI("Set speed: %d, %d", speed, tty_get_speed(speed));

	if (cfsetospeed(ti, tty_get_speed(speed)) < 0)
		return -errno;

	if (cfsetispeed(ti, tty_get_speed(speed)) < 0)
		return -errno;

	if (tcsetattr(fd, TCSANOW, ti) < 0)
		return -errno;

	return 0;
}

/*
 * Read an HCI event from the given file descriptor.
 */
int read_hci_event(int fd, unsigned char* buf, int size)
{
	int remain, r;
	int count = 0;

	if (size <= 0)
		return -1;

	/* The first byte identifies the packet type. For HCI event packets, it
	 * should be 0x04, so we read until we get to the 0x04. */
	while (1) {
		r = read(fd, buf, 1);
		if (r <= 0)
			return -1;
		if (buf[0] == 0x04)
			break;
	}
	count++;

	/* The next two bytes are the event code and parameter total length. */
	while (count < 3) {
		r = read(fd, buf + count, 3 - count);
		if (r <= 0)
			return -1;
		count += r;
	}

	/* Now we read the parameters. */
	if (buf[2] < (size - 3))
		remain = buf[2];
	else
		remain = size - 3;

	while ((count - 3) < remain) {
		r = read(fd, buf + count, remain - (count - 3));
		if (r <= 0)
			return -1;
		count += r;
	}

	return count;
}

static int bcm43xx(int fd, struct uart_t *u, struct termios *ti)
{
	return bcm43xx_init(fd, u->init_speed, u->speed, ti, u->bdaddr);
}

static struct uart_t * get_by_id(int m_id, int p_id)
{
	int i;
	for (i = 0; uart[i].type; i++) {
		if (uart[i].m_id == m_id && uart[i].p_id == p_id)
			return &uart[i];
	}
	return NULL;
}

static struct uart_t * get_by_type(char *type)
{
	int i;
	for (i = 0; uart[i].type; i++) {
		if (!strcmp(uart[i].type, type))
			return &uart[i];
	}
	return NULL;
}

/* Initialize UART driver */
static int init_uart(char *dev, struct uart_t *u, int send_break, int raw)
{
	struct termios ti;
	int fd, i;
	unsigned long flags = 0;

	if (raw) {
		flags |= 1 << HCI_UART_RAW_DEVICE;
	}

	if (u->flags & AMP_DEV)
		flags |= 1 << HCI_UART_CREATE_AMP;

	fd = open(dev, O_RDWR | O_NOCTTY);
	if (fd < 0) {
		ALOGE("Can't open serial port: %s(%s)", dev, strerror(errno));
		return -1;
	}

	tcflush(fd, TCIOFLUSH);

	if (tcgetattr(fd, &ti) < 0) {
		ALOGE("Can't get port settings");
		goto fail;
	}

	cfmakeraw(&ti);

	ti.c_cflag |= CLOCAL;
	if (u->flags & FLOW_CTL)
		ti.c_cflag |= CRTSCTS;
	else
		ti.c_cflag &= ~CRTSCTS;

	if (tcsetattr(fd, TCSANOW, &ti) < 0) {
		ALOGE("Can't set port settings");
		goto fail;
	}

	/* Set initial baudrate */
	if (set_speed(fd, &ti, u->init_speed) < 0) {
		ALOGE("Can't set initial baud rate");
		goto fail;
	}

	tcflush(fd, TCIOFLUSH);

	if (send_break) {
		tcsendbreak(fd, 0);
		usleep(500000);
	}

	if (u->init && u->init(fd, u, &ti) < 0)
		goto fail;

	tcflush(fd, TCIOFLUSH);

	/* Set actual baudrate */
	if (set_speed(fd, &ti, u->speed) < 0) {
		ALOGE("Can't set baud rate");
		goto fail;
	}

	/* Set TTY to N_HCI line discipline */
	i = N_HCI;
	if (ioctl(fd, TIOCSETD, &i) < 0) {
		ALOGE("Can't set line discipline");
		goto fail;
	}

	if (flags && ioctl(fd, HCIUARTSETFLAGS, flags) < 0) {
		ALOGE("Can't set UART flags");
		goto fail;
	}

	if (ioctl(fd, HCIUARTSETPROTO, u->proto) < 0) {
		ALOGE("Can't set device");
		goto fail;
	}

	if (u->post && u->post(fd, u, &ti) < 0)
		goto fail;

	return fd;

fail:
	close(fd);
	return -1;
}

// ---------------------------------------------------------------------------

#ifndef FIRMWARE_DIR
#define FIRMWARE_DIR "/etc/firmware"
#endif

#define HCI_COMMAND_PKT		0x01

#define FW_EXT ".hcd"

#define BCM43XX_CLOCK_48 1
#define BCM43XX_CLOCK_24 2

#define CMD_SUCCESS 0x00

#define CC_MIN_SIZE 7

#define MIN(X,Y) ((X) < (Y) ? (X) : (Y))


/* BD Address */
typedef struct {
	uint8_t b[6];
} __attribute__((packed)) bdaddr_t;

static int bachk(const char *str)
{
	if (!str)
		return -1;

	if (strlen(str) != 17)
		return -1;

	while (*str) {
		if (!isxdigit(*str++))
			return -1;

		if (!isxdigit(*str++))
			return -1;

		if (*str == 0)
			break;

		if (*str++ != ':')
			return -1;
	}

	return 0;
}

static int str2ba(const char *str, bdaddr_t *ba)
{
	int i;

	if (bachk(str) < 0) {
		memset(ba, 0, sizeof(*ba));
		return -1;
	}

	for (i = 5; i >= 0; i--, str += 3)
		ba->b[i] = strtol(str, NULL, 16);

	return 0;
}

static int bcm43xx_read_local_name(int fd, char *name, size_t size)
{
	unsigned char cmd[] = { HCI_COMMAND_PKT, 0x14, 0x0C, 0x00 };
	unsigned char *resp;
	unsigned int name_len;

	resp = malloc(size + CC_MIN_SIZE);
	if (!resp)
		return -1;

	tcflush(fd, TCIOFLUSH);

	if (write(fd, cmd, sizeof(cmd)) != sizeof(cmd)) {
		ALOGE("Failed to write read local name command");
		goto fail;
	}

	if (read_hci_event(fd, resp, size) < CC_MIN_SIZE) {
		ALOGE("Failed to read local name, invalid HCI event");
		goto fail;
	}

	if (resp[4] != cmd[1] || resp[5] != cmd[2] || resp[6] != CMD_SUCCESS) {
		ALOGE("Failed to read local name, command failure");
		goto fail;
	}

	name_len = resp[2] - 1;

	strncpy(name, (char *) &resp[7], MIN(name_len, size));
	name[size - 1] = 0;

	free(resp);
	return 0;

fail:
	free(resp);
	return -1;
}

static int bcm43xx_reset(int fd)
{
	unsigned char cmd[] = { HCI_COMMAND_PKT, 0x03, 0x0C, 0x00 };
	unsigned char resp[CC_MIN_SIZE];

	if (write(fd, cmd, sizeof(cmd)) != sizeof(cmd)) {
		ALOGE("Failed to write reset command");
		return -1;
	}

	if (read_hci_event(fd, resp, sizeof(resp)) < CC_MIN_SIZE) {
		ALOGE("Failed to reset chip, invalid HCI event");
		return -1;
	}

	if (resp[4] != cmd[1] || resp[5] != cmd[2] || resp[6] != CMD_SUCCESS) {
		ALOGE("Failed to reset chip, command failure");
		return -1;
	}

	return 0;
}

static int bcm43xx_set_bdaddr(int fd, const char *bdaddr)
{
	unsigned char cmd[] =
		{ HCI_COMMAND_PKT, 0x01, 0xfc, 0x06, 0x00, 0x00,
				0x00, 0x00, 0x00, 0x00 };
	unsigned char resp[CC_MIN_SIZE];

	ALOGI("Set BDADDR: %s", bdaddr);

	if (str2ba(bdaddr, (bdaddr_t *) (&cmd[4])) < 0) {
		ALOGE("Incorrect bdaddr");
		return -1;
	}

	tcflush(fd, TCIOFLUSH);

	if (write(fd, cmd, sizeof(cmd)) != sizeof(cmd)) {
		ALOGE("Failed to write set bdaddr command");
		return -1;
	}

	if (read_hci_event(fd, resp, sizeof(resp)) < CC_MIN_SIZE) {
		ALOGE("Failed to set bdaddr, invalid HCI event");
		return -1;
	}

	if (resp[4] != cmd[1] || resp[5] != cmd[2] || resp[6] != CMD_SUCCESS) {
		ALOGE("Failed to set bdaddr, command failure");
		return -1;
	}

	return 0;
}

static int bcm43xx_set_clock(int fd, unsigned char clock)
{
	unsigned char cmd[] = { HCI_COMMAND_PKT, 0x45, 0xfc, 0x01, 0x00 };
	unsigned char resp[CC_MIN_SIZE];

	ALOGI("Set Controller clock (%d)", clock);

	cmd[4] = clock;

	tcflush(fd, TCIOFLUSH);

	if (write(fd, cmd, sizeof(cmd)) != sizeof(cmd)) {
		ALOGE("Failed to write update clock command");
		return -1;
	}

	if (read_hci_event(fd, resp, sizeof(resp)) < CC_MIN_SIZE) {
		ALOGE("Failed to update clock, invalid HCI event");
		return -1;
	}

	if (resp[4] != cmd[1] || resp[5] != cmd[2] || resp[6] != CMD_SUCCESS) {
		ALOGE("Failed to update clock, command failure");
		return -1;
	}

	return 0;
}

static int bcm43xx_set_speed(int fd, struct termios *ti, uint32_t speed)
{
	unsigned char cmd[] =
		{ HCI_COMMAND_PKT, 0x18, 0xfc, 0x06, 0x00, 0x00,
				0x00, 0x00, 0x00, 0x00 };
	unsigned char resp[CC_MIN_SIZE];

	if (speed > 3000000 && bcm43xx_set_clock(fd, BCM43XX_CLOCK_48))
		return -1;

	ALOGI("Set Controller UART speed to %d bit/s", speed);

	cmd[6] = (uint8_t) (speed);
	cmd[7] = (uint8_t) (speed >> 8);
	cmd[8] = (uint8_t) (speed >> 16);
	cmd[9] = (uint8_t) (speed >> 24);

	tcflush(fd, TCIOFLUSH);

	if (write(fd, cmd, sizeof(cmd)) != sizeof(cmd)) {
		ALOGE("Failed to write update baudrate command");
		return -1;
	}

	if (read_hci_event(fd, resp, sizeof(resp)) < CC_MIN_SIZE) {
		ALOGE("Failed to update baudrate, invalid HCI event");
		return -1;
	}

	if (resp[4] != cmd[1] || resp[5] != cmd[2] || resp[6] != CMD_SUCCESS) {
		ALOGE("Failed to update baudrate, command failure");
		return -1;
	}

	if (set_speed(fd, ti, speed) < 0) {
		ALOGE("Can't set host baud rate");
		return -1;
	}

	return 0;
}

static int bcm43xx_load_firmware(int fd, const char *fw)
{
	unsigned char cmd[] = { HCI_COMMAND_PKT, 0x2e, 0xfc, 0x00 };
	struct timespec tm_mode = { 0, 50000 };
	struct timespec tm_ready = { 0, 2000000 };
	unsigned char resp[CC_MIN_SIZE];
	unsigned char tx_buf[1024];
	int len, fd_fw, n;

	ALOGI("Flash firmware %s", fw);

	fd_fw = open(fw, O_RDONLY);
	if (fd_fw < 0) {
		ALOGE("Unable to open firmware (%s)", fw);
		return -1;
	}

	tcflush(fd, TCIOFLUSH);

	if (write(fd, cmd, sizeof(cmd)) != sizeof(cmd)) {
		ALOGE("Failed to write download mode command");
		goto fail;
	}

	if (read_hci_event(fd, resp, sizeof(resp)) < CC_MIN_SIZE) {
		ALOGE("Failed to load firmware, invalid HCI event");
		goto fail;
	}

	if (resp[4] != cmd[1] || resp[5] != cmd[2] || resp[6] != CMD_SUCCESS) {
		ALOGE("Failed to load firmware, command failure");
		goto fail;
	}

	/* Wait 50ms to let the firmware placed in download mode */
	nanosleep(&tm_mode, NULL);

	tcflush(fd, TCIOFLUSH);

	while ((n = read(fd_fw, &tx_buf[1], 3))) {
		if (n < 0) {
			ALOGE("Failed to read firmware");
			goto fail;
		}

		tx_buf[0] = HCI_COMMAND_PKT;

		len = tx_buf[3];

		if (read(fd_fw, &tx_buf[4], len) < 0) {
			ALOGE("Failed to read firmware");
			goto fail;
		}

		if (write(fd, tx_buf, len + 4) != (len + 4)) {
			ALOGE("Failed to write firmware");
			goto fail;
		}

		read_hci_event(fd, resp, sizeof(resp));
		tcflush(fd, TCIOFLUSH);
	}

	/* Wait for firmware ready */
	nanosleep(&tm_ready, NULL);

	close(fd_fw);
	return 0;

fail:
	close(fd_fw);
	return -1;
}

static int bcm43xx_locate_patch(const char *dir_name,
		const char *chip_name, char *location)
{
	DIR *dir;
	int ret = -1;

	dir = opendir(dir_name);
	if (!dir) {
		ALOGE("Cannot open directory '%s': %s",
				dir_name, strerror(errno));
		return -1;
	}

	/* Recursively look for a BCM43XX*.hcd */
	while (1) {
		struct dirent *entry = readdir(dir);
		if (!entry)
			break;

		if (entry->d_type & DT_DIR) {
			char path[PATH_MAX];

			if (!strcmp(entry->d_name, "..") || !strcmp(entry->d_name, "."))
				continue;

			snprintf(path, PATH_MAX, "%s/%s", dir_name, entry->d_name);

			ret = bcm43xx_locate_patch(path, chip_name, location);
			if (!ret)
				break;
		} else if (!strncmp(chip_name, entry->d_name, strlen(chip_name))) {
			unsigned int name_len = strlen(entry->d_name);
			size_t curs_ext = name_len - sizeof(FW_EXT) + 1;

			if (curs_ext > name_len)
				break;

			if (strncmp(FW_EXT, &entry->d_name[curs_ext], sizeof(FW_EXT)))
				break;

			/* found */
			snprintf(location, PATH_MAX, "%s/%s", dir_name, entry->d_name);
			ret = 0;
			break;
		}
	}

	closedir(dir);

	return ret;
}

int bcm43xx_init(int fd, int def_speed, int speed, struct termios *ti,
		const char *bdaddr)
{
	char chip_name[20];
	char fw_path[PATH_MAX];

	ALOGI("bcm43xx_init ...");

	if (bcm43xx_reset(fd))
		return -1;

	if (bcm43xx_read_local_name(fd, chip_name, sizeof(chip_name)))
		return -1;

	ALOGI("bcm43xx_init chip name: %s", chip_name);

	if (bcm43xx_locate_patch(FIRMWARE_DIR, chip_name, fw_path)) {
		ALOGE("Patch not found, continue anyway");
	} else {
		if (bcm43xx_set_speed(fd, ti, speed))
			return -1;

		if (bcm43xx_load_firmware(fd, fw_path))
			return -1;

		usleep(500 * 1000);

		/* Controller speed has been reset to def speed */
		if (set_speed(fd, ti, def_speed) < 0) {
			ALOGE("Can't set host baud rate");
			return -1;
		}

		if (bcm43xx_reset(fd))
			return -1;
	}

	if (bdaddr)
		bcm43xx_set_bdaddr(fd, bdaddr);

	if (bcm43xx_set_speed(fd, ti, speed))
		return -1;

	return 0;
}
// ---------------------------------------------------------------------------

static void usage(const char *prog)
{
	ALOGI("%s - HCI UART driver initialization utility", prog);
	ALOGI("Usage:");
	ALOGI("\t%s [-s] [-r] [-t timeout] [-s initial_speed] "
		"<tty> <type | id> [speed] [flow|noflow] [sleep|nosleep] [bdaddr]", prog);
}

int main(int argc, char *argv[])
{
	struct uart_t *u = NULL;
	int opt, n, fd, raw = 0, ld, err;
	int to = 10;
	int send_break = 0;
	int init_speed = 0;
	struct sigaction sa;
	struct pollfd p;
	sigset_t sigs;
	char dev[PATH_MAX];

	while ((opt=getopt(argc, argv, "brt:s:")) != EOF) {
		switch(opt) {
		case 'b':
			send_break = 1;
			break;

		case 't':
			to = atoi(optarg);
			break;

		case 's':
			init_speed = atoi(optarg);
			break;

		case 'r':
			raw = 1;
			break;

		default:
			ALOGE("Failed to parse parameter.");
			usage(argv[0]);
			exit(1);
		}
	}

	n = argc - optind;
	if (n < 2) {
		ALOGE("n=%d", n);
		usage(argv[0]);
		exit(1);
	}

	for (n = 0; optind < argc; n++, optind++) {
		char *opt;

		opt = argv[optind];

		switch(n) {
		case 0:
			dev[0] = 0;
			if (!strchr(opt, '/'))
				strcpy(dev, "/dev/");

			if (strlen(opt) > PATH_MAX - (strlen(dev) + 1)) {
				ALOGE("Invalid serial device");
				exit(1);
			}

			strcat(dev, opt);
			break;

		case 1:
			if (strchr(argv[optind], ',')) {
				int m_id, p_id;
				sscanf(argv[optind], "%x,%x", &m_id, &p_id);
				u = get_by_id(m_id, p_id);
			} else {
				u = get_by_type(opt);
			}

			if (!u) {
				ALOGE("Unknown device type or id");
				exit(1);
			}
			break;

		case 2:
			u->speed = atoi(argv[optind]);
			break;

		case 3:
			if (!strcmp("flow", argv[optind]))
				u->flags |=  FLOW_CTL;
			else
				u->flags &= ~FLOW_CTL;
			break;

		case 4:
			if (!strcmp("sleep", argv[optind]))
				u->pm = ENABLE_PM;
			else
				u->pm = DISABLE_PM;
			break;

		case 5:
			{
				uint8_t b[3] = { 0x00, 0x00, 0x00 };
				int idx,  len;
				static char bdaddr[] = "b8:27:eb:90:75:b6";

				len  = strlen(argv[optind]);
				if (len >= 6) {
					char *p = argv[optind] + len - 6;

					for (idx = 0; idx < 6; idx++) {
						int ch = p[idx];
						int val = 0;
						if (ch >= '0' && ch <= '9') {
							val = ch - '0';
						} else if (ch >= 'A' && ch <= 'F') {
							val = ch - 'A' + 10;
						} else if (ch >= 'a' && ch <= 'f') {
							val = ch - 'a' + 10;
						}

						b[idx / 2] = b[idx / 2] * 16 + val;
					}
				}

				sprintf(bdaddr, "b8:27:eb:%02x:%02x:%02x", b[0] ^ 0xaa, b[1] ^ 0xaa, b[2] ^ 0xaa);

				u->bdaddr = bdaddr;
			}
			break;
		}
	}

	if (!u) {
		ALOGE("Unknown device type or id");
		exit(1);
	}

	/* If user specified a initial speed, use that instead of
	   the hardware's default */
	if (init_speed)
		u->init_speed = init_speed;

	memset(&sa, 0, sizeof(sa));
	sa.sa_flags   = SA_NOCLDSTOP;
	sa.sa_handler = sig_alarm;
	sigaction(SIGALRM, &sa, NULL);

	/* 10 seconds should be enough for initialization */
	alarm(to);

	fd = init_uart(dev, u, send_break, raw);
	if (fd < 0) {
		exit(1);
	}

	ALOGI("Device setup complete");

	alarm(0);

	memset(&sa, 0, sizeof(sa));
	sa.sa_flags   = SA_NOCLDSTOP;
	sa.sa_handler = SIG_IGN;
	sigaction(SIGCHLD, &sa, NULL);
	sigaction(SIGPIPE, &sa, NULL);

	sa.sa_handler = sig_term;
	sigaction(SIGTERM, &sa, NULL);
	sigaction(SIGINT,  &sa, NULL);

	sa.sa_handler = sig_hup;
	sigaction(SIGHUP, &sa, NULL);

	p.fd = fd;
	p.events = POLLERR | POLLHUP;

	sigfillset(&sigs);
	sigdelset(&sigs, SIGCHLD);
	sigdelset(&sigs, SIGPIPE);
	sigdelset(&sigs, SIGTERM);
	sigdelset(&sigs, SIGINT);
	sigdelset(&sigs, SIGHUP);

	while (!__io_canceled) {
		p.revents = 0;
		err = ppoll(&p, 1, NULL, &sigs);
		if (err < 0 && errno == EINTR)
			continue;
		if (err)
			break;
	}

	/* Restore TTY line discipline */
	ld = N_TTY;
	if (ioctl(n, TIOCSETD, &ld) < 0) {
		ALOGE("Can't restore line discipline");
		exit(1);
	}

	return 0;
}
