/*
 * ntlmv1scan - detect NTLMv1 authentication messages in Linux network traffic
 * Author: Gratien Dhaese contributors
 * License: GPL v3
 */

#include <arpa/inet.h>
#include <linux/if_packet.h>
#include <linux/if_ether.h>
#include <net/if.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>
#include <time.h>

struct scan_stats {
	unsigned long packets;
	unsigned long ntlm_auth_messages;
	unsigned long ntlmv1_hits;
};

static const unsigned char ntlmssp_signature[] = "NTLMSSP\0";

static uint16_t read_le16(const unsigned char *p)
{
	return (uint16_t)(p[0] | (p[1] << 8));
}

static uint32_t read_le32(const unsigned char *p)
{
	return (uint32_t)(p[0] | (p[1] << 8) | (p[2] << 16) | (p[3] << 24));
}

static void format_timestamp(const struct timeval *ts, char *buffer, size_t buffer_len)
{
	struct tm tm_value;

	if (localtime_r(&ts->tv_sec, &tm_value) == NULL) {
		(void)snprintf(buffer, buffer_len, "time-unavailable");
		return;
	}

	if (strftime(buffer, buffer_len, "%Y-%m-%d %H:%M:%S", &tm_value) == 0) {
		(void)snprintf(buffer, buffer_len, "time-unavailable");
	}
}

static void inspect_ntlm_payload(const unsigned char *payload, size_t payload_len, const struct timeval *ts, struct scan_stats *stats)
{
	size_t start = 0;
	const size_t min_auth_size = 28;
	char when[64];

	while (start + 12 <= payload_len) {
		size_t i;
		int found = 0;

		for (i = start; i + 12 <= payload_len; i++) {
			if (memcmp(payload + i, ntlmssp_signature, 8) == 0 && read_le32(payload + i + 8) == 3U) {
				found = 1;
				break;
			}
		}

		if (!found) {
			break;
		}

		stats->ntlm_auth_messages++;
		if (i + min_auth_size <= payload_len) {
			uint16_t lm_response_len = read_le16(payload + i + 12);
			uint16_t nt_response_len = read_le16(payload + i + 20);

			if (nt_response_len == 24U) {
				stats->ntlmv1_hits++;
				format_timestamp(ts, when, sizeof(when));
				(void)printf(
					"[%s.%06ld] Potential NTLMv1 authentication detected (packet=%lu, lm_len=%u, nt_len=%u)\n",
					when,
					(long)ts->tv_usec,
					stats->packets,
					(unsigned int)lm_response_len,
					(unsigned int)nt_response_len);
			}
		}

		start = i + 8;
	}
}

static int is_smb_port(uint16_t port)
{
	return (port == 139U || port == 445U);
}

static void process_frame(const unsigned char *frame, ssize_t frame_len, const struct timeval *ts, struct scan_stats *stats)
{
	const struct ethhdr *eth;
	const struct iphdr *ip;
	const struct tcphdr *tcp;
	const unsigned char *payload;
	size_t ip_len;
	size_t tcp_len;
	size_t payload_len;

	if (frame_len < (ssize_t)sizeof(struct ethhdr)) {
		return;
	}

	eth = (const struct ethhdr *)frame;
	if (ntohs(eth->h_proto) != ETH_P_IP) {
		return;
	}

	if (frame_len < (ssize_t)(sizeof(struct ethhdr) + sizeof(struct iphdr))) {
		return;
	}

	ip = (const struct iphdr *)(frame + sizeof(struct ethhdr));
	if (ip->protocol != IPPROTO_TCP) {
		return;
	}

	ip_len = (size_t)ip->ihl * 4U;
	if (ip_len < sizeof(struct iphdr)) {
		return;
	}
	if (frame_len < (ssize_t)(sizeof(struct ethhdr) + ip_len + sizeof(struct tcphdr))) {
		return;
	}

	tcp = (const struct tcphdr *)(frame + sizeof(struct ethhdr) + ip_len);
	if (!is_smb_port(ntohs(tcp->source)) && !is_smb_port(ntohs(tcp->dest))) {
		return;
	}

	tcp_len = (size_t)tcp->doff * 4U;
	if (tcp_len < sizeof(struct tcphdr)) {
		return;
	}
	if (frame_len < (ssize_t)(sizeof(struct ethhdr) + ip_len + tcp_len)) {
		return;
	}

	payload = frame + sizeof(struct ethhdr) + ip_len + tcp_len;
	payload_len = (size_t)frame_len - (sizeof(struct ethhdr) + ip_len + tcp_len);
	if (payload_len == 0U) {
		return;
	}

	inspect_ntlm_payload(payload, payload_len, ts, stats);
}

static void usage(const char *prog)
{
	(void)fprintf(stderr,
		      "Usage: %s -i interface [-c packet_count]\n"
		      "  -i interface     Capture live traffic from interface (root required)\n"
		      "  -c packet_count  Stop after packet_count packets\n",
		      prog);
}

int main(int argc, char **argv)
{
	char *interface_name = NULL;
	int packet_count = 0;
	int opt;
	int sockfd;
	unsigned char buffer[65536];
	struct scan_stats stats = {0, 0, 0};

	while ((opt = getopt(argc, argv, "i:c:h")) != -1) {
		switch (opt) {
		case 'i':
			interface_name = optarg;
			break;
		case 'c':
			packet_count = atoi(optarg);
			if (packet_count < 0) {
				(void)fprintf(stderr, "Invalid packet count: %s\n", optarg);
				return EXIT_FAILURE;
			}
			break;
		case 'h':
		default:
			usage(argv[0]);
			return (opt == 'h') ? EXIT_SUCCESS : EXIT_FAILURE;
		}
	}

	if (interface_name == NULL) {
		usage(argv[0]);
		return EXIT_FAILURE;
	}

	sockfd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
	if (sockfd < 0) {
		perror("socket");
		return EXIT_FAILURE;
	}

	{
		struct ifreq ifr;
		struct sockaddr_ll sll;

		memset(&ifr, 0, sizeof(ifr));
		(void)strncpy(ifr.ifr_name, interface_name, IFNAMSIZ - 1);
		if (ioctl(sockfd, SIOCGIFINDEX, &ifr) < 0) {
			perror("SIOCGIFINDEX");
			close(sockfd);
			return EXIT_FAILURE;
		}

		memset(&sll, 0, sizeof(sll));
		sll.sll_family = AF_PACKET;
		sll.sll_protocol = htons(ETH_P_ALL);
		sll.sll_ifindex = ifr.ifr_ifindex;
		if (bind(sockfd, (struct sockaddr *)&sll, sizeof(sll)) < 0) {
			perror("bind");
			close(sockfd);
			return EXIT_FAILURE;
		}
	}

	(void)printf("Scanning interface '%s' for NTLMv1 authentication traffic...\n", interface_name);
	while (packet_count == 0 || (int)stats.packets < packet_count) {
		ssize_t frame_len;
		struct timeval now;

		frame_len = recvfrom(sockfd, buffer, sizeof(buffer), 0, NULL, NULL);
		if (frame_len <= 0) {
			perror("recvfrom");
			close(sockfd);
			return EXIT_FAILURE;
		}

		stats.packets++;
		if (gettimeofday(&now, NULL) != 0) {
			memset(&now, 0, sizeof(now));
		}
		process_frame(buffer, frame_len, &now, &stats);
	}

	close(sockfd);
	(void)printf("\nScan summary:\n");
	(void)printf("  packets processed          : %lu\n", stats.packets);
	(void)printf("  NTLM authenticate messages : %lu\n", stats.ntlm_auth_messages);
	(void)printf("  potential NTLMv1 hits      : %lu\n", stats.ntlmv1_hits);

	return (stats.ntlmv1_hits > 0) ? EXIT_FAILURE : EXIT_SUCCESS;
}
