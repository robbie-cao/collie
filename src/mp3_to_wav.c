/**
 * Convert mp3 to wav
 *
 * cc mp3_to_wav.c -lmpg123 -lsndfile -o mp3_towav
 */

#include <stdio.h>
#include <string.h>

#include <mpg123.h>
#include <sndfile.h>

static const char *modes[5]    = { "Stereo", "Joint-Stereo", "Dual-Channel", "Single-Channel", "Invalid" };
static const char *smodes[5]   = { "stereo", "j-s", "dual", "mono", "o.O" };
static const char *layers[4]   = { "Unknown" , "I", "II", "III" };
static const char *versions[4] = { "1.0", "2.0", "2.5", "x.x" };
static const int samples_per_frame[4][4] =
{
    { -1 , 384 , 1152 , 1152 } , /* MPEG 1 */
    { -1 , 384 , 1152 , 576  } , /* MPEG 2 */
    { -1 , 384 , 1152 , 576  } , /* MPEG 2.5 */
    { -1 , -1  , -1   , -1   } , /* Unknown */
};

void print_header(mpg123_handle *mh)
{
    struct mpg123_frameinfo i;

    mpg123_info(mh, &i);
    if (i.mode > 4 || i.mode < 0) {
        i.mode = 4;
    }
    if (i.version > 3 || i.version < 0) {
        i.version = 3;
    }
    if (i.layer > 3 || i.layer < 0) {
        i.layer = 0;
    }
    fprintf(stderr,"MPEG %s, Layer: %s, Freq: %ld, Mode: %s, Modext: %d, BPF : %d\n",
            versions[i.version],
            layers[i.layer],
            i.rate,
            modes[i.mode],
            i.mode_ext,
            i.framesize);
    fprintf(stderr,"Channels: %d, Copyright: %s, Original: %s, CRC: %s, Emphasis: %d.\n",
            i.mode == MPG123_M_MONO ? 1 : 2,
            i.flags & MPG123_COPYRIGHT ? "Yes" : "No",
            i.flags & MPG123_ORIGINAL ? "Yes" : "No",
            i.flags & MPG123_CRC ? "Yes" : "No",
            i.emphasis);
    fprintf(stderr,"Bitrate: ");
    switch (i.vbr) {
        case MPG123_CBR:
            if (i.bitrate) {
                fprintf(stderr, "%d kbit/s", i.bitrate);
            } else {
                fprintf(stderr, "%d kbit/s (free format)", (int)((double)(i.framesize+4)*8*i.rate*0.001/samples_per_frame[i.version][i.layer]+0.5));
            }
            break;
        case MPG123_VBR:
            fprintf(stderr, "VBR");
            break;
        case MPG123_ABR:
            fprintf(stderr, "%d kbit/s ABR", i.abr_rate);
            break;
        default:
            fprintf(stderr, "???");
            break;
    }
    fprintf(stderr, "\nExtension value: %d\n", i.flags & MPG123_PRIVATE ? 1 : 0);
}

static void usage()
{
    printf("Usage: mp3_to_wav <input> <output> [s16|f32 [<buffersize>]]\n");
}

static void cleanup(mpg123_handle *mh)
{
    /* It's really to late for error checks here;-) */
    mpg123_close(mh);
    mpg123_delete(mh);
    mpg123_exit();
}

int main(int argc, char *argv[])
{
    SNDFILE* sndfile = NULL;
    SF_INFO sfinfo;
    mpg123_handle *mh = NULL;
    unsigned char* buffer = NULL;
    size_t buffer_size = 0;
    size_t done = 0;
    int  channels = 0, encoding = 0;
    long rate = 0;
    int  err  = MPG123_OK;
    off_t samples = 0;

    if (argc < 3) {
        usage();
        return 1;
    }

    printf("Input file: %s\n", argv[1]);
    printf("Output file: %s\n", argv[2]);

    err = mpg123_init();
    if (err != MPG123_OK) {
        fprintf(stderr, "Fail to init mpg123: %s", mpg123_plain_strerror(err));
        return -1;
    }
    mh = mpg123_new(NULL, &err);
    if (err != MPG123_OK || mh == NULL) {
        fprintf(stderr, "Basic setup goes wrong: %s", mpg123_plain_strerror(err));
        cleanup(mh);
        return -1;
    }

    mpg123_param(mh, MPG123_RESYNC_LIMIT, -1, 0);
    mpg123_param(mh, MPG123_INDEX_SIZE, -1, 0);

    /* Simple hack to enable floating point output. */
    if (argc >= 4 && !strcmp(argv[3], "f32")) {
        mpg123_param(mh, MPG123_ADD_FLAGS, MPG123_FORCE_FLOAT, 0.);
    }

    if (    mpg123_open(mh, argv[1]) != MPG123_OK
            /* Let mpg123 work with the file, that excludes MPG123_NEED_MORE messages. */
            || mpg123_getformat(mh, &rate, &channels, &encoding) != MPG123_OK
            /* Peek into track and get first output format. */
       ) {
        fprintf(stderr, "Trouble with mpg123: %s\n", mpg123_strerror(mh));
        cleanup(mh);
        return -1;
    }

    printf("Rate     : %li\n", rate);
    printf("Channel  : %d\n", channels);
    printf("Encodeing: %#08X\n", encoding);
    print_header(mh);

    if (encoding != MPG123_ENC_SIGNED_16 && encoding != MPG123_ENC_FLOAT_32) {
        /*
         * Signed 16 is the default output format anyways; it would actually by only different if we forced it.
         * So this check is here just for this explanation.
         */
        cleanup(mh);
        fprintf(stderr, "Bad encoding: 0x%x!\n", encoding);
        return -2;
    }

    /*
     * Scan mp3 file and dump its seek index.
     */
    off_t* offsets;
    off_t step;
    size_t fill, i;

    mpg123_scan(mh);
    mpg123_index(mh, &offsets, &step, &fill);

    printf("Total frames: %li\n", fill);
#if 0
    for (i = 0; i < fill; i++) {
        printf("Frame number %li: file offset %li\n", i * step, offsets[i]);
    }
#endif
    printf("Total samples: %li\n", mpg123_length(mh));

    /* Ensure that this output format will not change (it could, when we allow it). */
    mpg123_format_none(mh);
    mpg123_format(mh, rate, channels, encoding);

    bzero(&sfinfo, sizeof(sfinfo));
    sfinfo.samplerate = rate;
    sfinfo.channels = channels;
    sfinfo.format = SF_FORMAT_WAV | (encoding == MPG123_ENC_SIGNED_16 ? SF_FORMAT_PCM_16 : SF_FORMAT_FLOAT);
    printf("Creating WAV with %i channels and %li Hz.\n", channels, rate);

    sndfile = sf_open(argv[2], SFM_WRITE, &sfinfo);
    if (sndfile == NULL) {
        fprintf(stderr, "Cannot open output file!\n");
        cleanup(mh);
        return -2;
    }

    /*
     * Buffer could be almost any size here, mpg123_outblock() is just some recommendation.
     * Important, especially for sndfile writing, is that the size is a multiple of sample size.
     */
    buffer_size = (argc >= 5) ? atol(argv[4]) : mpg123_outblock(mh);
    printf("Buffer size: %li\n", buffer_size);
    buffer = malloc(buffer_size);
    if (buffer == NULL) {
        fprintf(stderr, "Fail to allocate buffer!\n");
        sf_close(sndfile);
        cleanup(mh);
        return -3;
    }

    do {
        sf_count_t more_samples;

        err = mpg123_read(mh, buffer, buffer_size, &done);
        more_samples = (encoding == MPG123_ENC_SIGNED_16)
            ? sf_write_short(sndfile, (short*)buffer, done / sizeof(short))
            : sf_write_float(sndfile, (float*)buffer, done / sizeof(float));
        if (more_samples < 0 || more_samples * mpg123_encsize(encoding) != done) {
            fprintf(stderr, "Warning: Written number of samples does not match the byte count we got from libmpg123: %li != %li\n",
                    (long)(more_samples*mpg123_encsize(encoding)),
                    (long)done);
        }
        samples += more_samples;
        /*
         * We are not in feeder mode, so MPG123_OK, MPG123_ERR and MPG123_NEW_FORMAT are the only possibilities.
         * We do not handle a new format, MPG123_DONE is the end... so abort on anything not MPG123_OK.
         */
    } while (err == MPG123_OK);

    if (err != MPG123_DONE) {
        fprintf(stderr, "Warning: Decoding ended prematurely because: %s\n",
                err == MPG123_ERR ? mpg123_strerror(mh) : mpg123_plain_strerror(err));
    }

    samples /= channels;
    printf("%li samples written.\n", (long)samples);

    free(buffer);
    sf_close(sndfile);
    cleanup(mh);

    return 0;
}
