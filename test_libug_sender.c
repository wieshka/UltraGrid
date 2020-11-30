#include <unistd.h>

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <libug.h>

#define WIDTH 1920
#define HEIGHT 1080
#define FPS 30.0

static void render_packet_received_callback(void *udata, struct RenderPacket *pkt) {
        (void) udata;
        printf("Received RenderPacket: %p\n", pkt);
}

static void usage(const char *progname) {
        printf("%s [options] [receiver[:port]]\n", progname);
        printf("options:\n");
        printf("\t-h - show this help\n");
        printf("\t-j - use JPEG\n");
}

static void fill(unsigned char *data, int width, int height, libug_pixfmt_t pixfmt) {
        assert(pixfmt == UG_RGBA);
        for (int y = 0; y < height; ++y) {
                for (int x = 0; x < width; ++x) {
                        *data++ = x % 256;
                        *data++ = x % 256;
                        *data++ = x % 256;
                        *data++ = 255;
                }
        }
}

int main(int argc, char *argv[]) {
        struct ug_sender_parameters init_params = { 0 };
        init_params.receiver = "localhost";
        init_params.compression = UG_UNCOMPRESSED;
        init_params.rprc = render_packet_received_callback;
        init_params.rprc_udata = NULL; // not used by render_packet_received_callback()

        int ch = 0;
        while ((ch = getopt(argc, argv, "hj")) != -1) {
                switch (ch) {
                case 'h':
                        usage(argv[0]);
                        return 0;
                case 'j':
                        init_params.compression = UG_JPEG;
                        break;
                default:
                        usage(argv[0]);
                        return -2;
                }
        }

        argc -= optind;
        argv += optind;

        if (argc > 0) {
                init_params.receiver = argv[0];
                if (strchr(argv[0], ':') != NULL) {
                        char *port_str = strchr(argv[0], ':') + 1;
                        *strchr(argv[1], ':') = '\0';
                        init_params.tx_port = atoi(port_str);
                }
        }

        struct ug_sender *s = ug_sender_init(&init_params);
        if (!s) {
                return 1;
        }
        time_t t0 = time(NULL);
        size_t len = WIDTH * HEIGHT * 4;
        unsigned char *test = malloc(len);
        fill(test, WIDTH, HEIGHT, UG_RGBA);
        int frames = 0;
        while (1) {
                ug_send_frame(s, test, UG_RGBA, WIDTH, HEIGHT);
                frames += 1;
                time_t seconds = time(NULL) - t0;
                if (seconds > 0) {
                        printf("Sent %d frames in last %ld second%s.\n", frames, seconds, seconds > 1 ? "s" : "");
                        t0 += seconds;
                        frames = 0;
                }
                usleep(1000 * 1000 / FPS);
        }

        free(test);
        ug_sender_done(s);
}
