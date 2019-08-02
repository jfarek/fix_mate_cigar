#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "getopt.h"
#include "htslib/hts.h"
#include "htslib/sam.h"

void usage()
{
    fprintf(stderr, "Remove MC tag from reads marked as mate unmapped.\n\n");
    fprintf(stderr, "Usage: fix_mate_cigar [-i input.bam] [-o output.bam] [-c compression_level]\n");
    fprintf(stderr, "Specify \"-i -\" to read from stdin and \"-o -\" to write to stdout.\n");
}

int main(int argc, char **argv)
{
    int c;
    uint8_t compression_level, tmpi, *aux;
    char *end, *input_fn, *output_fn, open_mode[4], write_mode[4], mc[2];

    input_fn = NULL;
    output_fn = NULL;
    compression_level = 6;

    if (argc == 1) {
        usage();
        return 0;
    }

    while ((c = getopt(argc, argv, "c:hi:o:")) != -1) {
        switch(c) {
        case 'c':
            tmpi = (uint8_t)strtol(optarg, &end, 10);
            if (tmpi >= 0 && tmpi <= 9) {
                compression_level = tmpi;
            }
            break;
        case 'h':
            usage();
            return 0;
            break;
        case 'i':
            input_fn = optarg;
            break;
        case 'o':
            output_fn = optarg;
            break;
        default:
            break;
        }
    }

    if (input_fn == NULL) {
        fprintf(stderr, "Error: Input BAM not specified\n");
        return EXIT_FAILURE;
    }

    if (output_fn == NULL) {
        fprintf(stderr, "Error: Output BAM not specified\n");
        return EXIT_FAILURE;
    }

    /* Setup reader */

    open_mode[0] = 'r';
    open_mode[1] = 'b';
    open_mode[2] = '0' + compression_level;
    open_mode[3] = '\0';

    htsFile *bam_reader = hts_open(input_fn, open_mode);
    if (bam_reader == NULL) {
        fprintf(stderr, "Error: Unable to read input BAM %s\n", input_fn);
        return EXIT_FAILURE;
    }

    bam_hdr_t *bam_header = sam_hdr_read(bam_reader);
    if (bam_header == NULL) {
        fprintf(stderr, "Error: Unable to read header of input BAM %s\n", input_fn);
        return EXIT_FAILURE;
    }

    bam1_t *bam_record = bam_init1();
    if (bam_record == NULL) {
        fprintf(stderr, "Error: Unable to initialize bam1_t record\n");
        return EXIT_FAILURE;
    }

    /* Setup writer */

    write_mode[0] = 'w';
    write_mode[1] = 'b';
    write_mode[2] = '0' + compression_level;
    write_mode[3] = '\0';

    htsFile *bam_writer = hts_open(output_fn, write_mode);
    if (bam_writer == NULL) {
        fprintf(stderr, "Error: Unable to open writer for output BAM %s\n", output_fn);
        return EXIT_FAILURE;
    }

    if (sam_hdr_write(bam_writer, bam_header) < 0) {
        fprintf(stderr, "Error: Unable to writer header for output BAM %s\n", output_fn);
        return EXIT_FAILURE;
    }

    /* Modify reads */

    mc[0] = 'M';
    mc[1] = 'C';

    while (sam_read1(bam_reader, bam_header, bam_record) >= 0) {
        /**
         * Correct invalid presence of MC tag:
         * If record is filtered as mate unmapped and an MC tag is present, delete the MC tag.
         * This corresponds to the Picard ValidateSam error MATE_CIGAR_STRING_INVALID_PRESENCE.
         */
        if (bam_record->core.flag & BAM_FMUNMAP && (aux = bam_aux_get(bam_record, mc)) != NULL) {
            bam_aux_del(bam_record, aux);
        }

        if (sam_write1(bam_writer, bam_header, bam_record) < 0) {
            fprintf(stderr, "Error: Unable to write BAM record to output BAM %s\n", output_fn);
            return EXIT_FAILURE;
        }
    }

    /* Cleanup */

    sam_close(bam_writer);
    bam_destroy1(bam_record);
    bam_hdr_destroy(bam_header);
    sam_close(bam_reader);

    return EXIT_SUCCESS;
}
