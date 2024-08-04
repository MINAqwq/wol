#include <arpa/inet.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>

#define print(str) write(STDERR_FILENO, str, sizeof(str) - 1)

static uint32_t ip_addr = 0xFFFFFFFF; /* 255.255.255.255 by default */
static uint16_t port = 9;	      /* discard port by default */

/* 48 bit media access control address stored in network order */
typedef struct {
	uint8_t octets[6];
} MacAddress;

typedef struct {
	uint8_t	   magic[6];
	MacAddress data[16];
} WolPackage;

static const char str_help_msg[] = "======================================\n"
				   "USAGE:\n"
				   "  wol [mac]\n"
				   "\n"
				   "  wol A3:43:F5:87:01:3B\n"
				   "======================================\n";

static const char str_wake_up[] = "WAKE UP!\n";

void
print_help(void)
{
	/* size of str_help_msg is known at compile time so no need to calculate
	 * this shit again */
	print(str_help_msg);
}

uint8_t
is_hexdigit(const char s)
{
	return ((s >= '0' && s <= '9') || (s >= 'A' && s <= 'F') ||
		(s >= 'a' && s <= 'f'));
}

/* only use this function after checking with is_hexdigit */
uint8_t
hexchar_to_bin(const char d)
{
	return (uint8_t)((d >= 'a') ? (d - 0x57)
				    : (d - ((d >= 'A') ? 0x37 : 0x30)));
}

/* convert a 48 bit mac address string with ':' notation to binary data. returns
 * 1 if successfull, otherwise 0 */
uint8_t
str_to_mac(char *str_mac, MacAddress *buffer_addr)
{
	uint8_t i;
	char	d0, d1, sep;

	if (!str_mac) {
		return 0;
	}

	for (i = 0; i < 6; i++) {
		d1 = *str_mac;
		if (!is_hexdigit(d1)) {
			return 0;
		}

		str_mac++;
		d0 = *str_mac;

		if (!is_hexdigit(d0)) {
			return 0;
		}

		str_mac++;
		sep = *str_mac;

		if ((sep != ':') && (sep != 0)) {
			return 0;
		}

		str_mac++;

		buffer_addr->octets[i] = hexchar_to_bin(d0);
		buffer_addr->octets[i] |= (hexchar_to_bin(d1) << 4);
	}

	return 1;
}

uint8_t
wol_send(struct in_addr *ip, uint16_t port, MacAddress addr)
{
	struct sockaddr_in sock_addr;
	WolPackage	   wol_package;
	int		   i, sd, broadcast_enable;

	sd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sd == -1) {
		perror("socket");
		return 1;
	}

	broadcast_enable = 1;
	if (setsockopt(sd, SOL_SOCKET, SO_BROADCAST, &broadcast_enable,
		       sizeof(broadcast_enable)) != 0) {
		perror("setsockopt");
		return 1;
	}

	for (i = 0; i < 6; i++) {
		wol_package.magic[i] = 0xFF;
	}

	for (i = 0; i < 16; i++) {
		wol_package.data[i] = addr;
	}

	sock_addr.sin_family = AF_INET;
	sock_addr.sin_port = htons(port);
	sock_addr.sin_addr = *ip;

	if (sendto(sd, &wol_package, sizeof(wol_package), 0,
		   (const struct sockaddr *)&sock_addr,
		   sizeof(sock_addr)) == -1) {
		perror("sendto");
		return 1;
	}

	print(str_wake_up);

	close(sd);

	return 0;
}

int
main(int argc, char **argv)
{
	struct in_addr ip;
	MacAddress     mac;

	if (argc != 2) {
		print_help();
		return 1;
	}

	if (!str_to_mac(argv[1], &mac)) {
		print_help();
		return 1;
	}

	ip.s_addr = ip_addr;
	return (int)wol_send(&ip, port, mac);
}
