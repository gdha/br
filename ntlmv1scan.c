/*
 * ntlmv1scan - detect NTLMv1 authentication messages in captured traffic
 * Author: Gratien Dhaese contributors
 * License: GPL v3
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

#if defined(__has_include)
#  if __has_include(<pcap/pcap.h>)
#    include <pcap/pcap.h>
#  elif __has_include(<pcap.h>)
#    include <pcap.h>
#  else
#    error "libpcap headers not found"
#  endif
#else
#  include <pcap/pcap.h>
#endif

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

static void inspect_ntlm_payload(const struct pcap_pkthdr *header, const unsigned char *packet, struct scan_stats *stats)
{
	size_t start = 0;
	const size_t min_auth_size = 28;
	char when[64];

	while (start + 12 <= header->caplen) {
		size_t i;
		int found = 0;

		for (i = start; i + 12 <= header->caplen; i++) {
			if (memcmp(packet + i, ntlmssp_signature, 8) == 0 && read_le32(packet + i + 8) == 3U) {
				found = 1;
				break;
			}
		}

		if (!found) {
			break;
		}

		stats->ntlm_auth_messages++;
		if (i + min_auth_size <= header->caplen) {
			uint16_t lm_response_len = read_le16(packet + i + 12);
			uint16_t nt_response_len = read_le16(packet + i + 20);

			if (nt_response_len == 24U) {
				stats->ntlmv1_hits++;
				format_timestamp(&header->ts, when, sizeof(when));
				(void)printf(
					"[%s.%06ld] Potential NTLMv1 authentication detected (packet=%lu, lm_len=%u, nt_len=%u)\n",
					when,
					(long)header->ts.tv_usec,
					stats->packets,
					(unsigned int)lm_response_len,
					(unsigned int)nt_response_len);
			}
		}

		start = i + 8;
	}
}

static void packet_handler(unsigned char *user_data, const struct pcap_pkthdr *header, const unsigned char *packet)
{
	struct scan_stats *stats = (struct scan_stats *)user_data;

	stats->packets++;
	inspect_ntlm_payload(header, packet, stats);
}

static void usage(const char *prog)
{
	(void)fprintf(stderr,
		      "Usage: %s [-i interface | -r capture.pcap] [-c packet_count]\n"
		      "  -i interface     Capture live traffic from interface\n"
		      "  -r capture.pcap  Read packets from pcap file\n"
		      "  -c packet_count  Stop after packet_count packets\n",
		      prog);
}

int main(int argc, char **argv)
{
	char errbuf[PCAP_ERRBUF_SIZE];
	char *interface_name = NULL;
	char *capture_file = NULL;
	int packet_count = 0;
	int opt;
	pcap_t *handle;
	struct bpf_program fp;
	struct scan_stats stats = {0, 0, 0};
	const char *filter_expression = "tcp port 139 or tcp port 445";

	while ((opt = getopt(argc, argv, "i:r:c:h")) != -1) {
		switch (opt) {
		case 'i':
			interface_name = optarg;
			break;
		case 'r':
			capture_file = optarg;
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

	if ((interface_name == NULL && capture_file == NULL) || (interface_name != NULL && capture_file != NULL)) {
		usage(argv[0]);
		return EXIT_FAILURE;
	}

	if (capture_file != NULL) {
		handle = pcap_open_offline(capture_file, errbuf);
	} else {
		handle = pcap_open_live(interface_name, BUFSIZ, 1, 1000, errbuf);
	}

	if (handle == NULL) {
		(void)fprintf(stderr, "pcap open failed: %s\n", errbuf);
		return EXIT_FAILURE;
	}

	if (pcap_compile(handle, &fp, filter_expression, 1, PCAP_NETMASK_UNKNOWN) == -1) {
		(void)fprintf(stderr, "pcap compile failed: %s\n", pcap_geterr(handle));
		pcap_close(handle);
		return EXIT_FAILURE;
	}

	if (pcap_setfilter(handle, &fp) == -1) {
		(void)fprintf(stderr, "pcap setfilter failed: %s\n", pcap_geterr(handle));
		pcap_freecode(&fp);
		pcap_close(handle);
		return EXIT_FAILURE;
	}
	pcap_freecode(&fp);

	(void)printf("Scanning for NTLMv1 authentication traffic...\n");
	if (pcap_loop(handle, packet_count, packet_handler, (unsigned char *)&stats) == -1) {
		(void)fprintf(stderr, "pcap loop failed: %s\n", pcap_geterr(handle));
		pcap_close(handle);
		return EXIT_FAILURE;
	}

	pcap_close(handle);
	(void)printf("\nScan summary:\n");
	(void)printf("  packets processed          : %lu\n", stats.packets);
	(void)printf("  NTLM authenticate messages : %lu\n", stats.ntlm_auth_messages);
	(void)printf("  potential NTLMv1 hits      : %lu\n", stats.ntlmv1_hits);

	return (stats.ntlmv1_hits > 0) ? EXIT_FAILURE : EXIT_SUCCESS;
}
