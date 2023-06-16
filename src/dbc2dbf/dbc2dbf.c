/* dbc2dbf.c
   Copyright (C) 2016 Daniela Petruzalek

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU Affero General Public License as published
   by the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

/*
    Author Notes
    ============

    This program decompresses .dbc files to .dbf. This code is based on the work
    of Mark Adler <madler@alumni.caltech.edu> (zlib/blast) and Pablo Fonseca
    (https://github.com/eaglebh/blast-dbf).
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdint.h>

#include "blast.h"

#define CHUNK 4096

/* Input file helper function */
static unsigned inf(void *how, unsigned char **buf) {
    static unsigned char hold[CHUNK];

    *buf = hold;
    return fread(hold, 1, CHUNK, (FILE *)how);
}

/* Output file helper function */
static int outf(void *how, unsigned char *buf, unsigned len) {
    return fwrite(buf, 1, len, (FILE *)how) != len;
}


/*
    dbc2dbf(char** input_file, char** output_file)
    This function decompresses a given .dbc input file into the corresponding .dbf.

    Please provide fully qualified names, including file extension.
 */
int dbc2dbf(char* input_file, char* output_file) {
    FILE          *input = 0, *output = 0;
    int           ret = 0;
    unsigned char rawHeader[2];
    uint16_t      headerSize = 0;
    unsigned char *buf = 0;

    input = fopen(input_file, "rb");
    if(input == NULL) {
        ret = errno;
        fprintf(stderr, "error opening input file %s: %s", input_file, strerror(ret));
        goto clean;
    }

    output = fopen(output_file, "wb");
    if(output == NULL) {
        ret = errno;
        fprintf(stderr, "error opening output file %s: %s", output_file, strerror(ret));
        goto clean;
    }

    /* Process file header - skip 8 bytes */
    if( fseek(input, 8, SEEK_SET) ) {
        ret = errno;
        fprintf(stderr, "error seeking input file %s: %s", input_file, strerror(ret));
        goto clean;
    }

    /* Reads two bytes from the header = header size */
    ret = fread(rawHeader, 2, 1, input);
    if( ferror(input) ) {
        fprintf(stderr, "error reading input file %s: %s", input_file, strerror(errno));
        goto clean;
    }

    /* Platform independent code (header size is stored in little endian format) */
    headerSize = rawHeader[0] + (rawHeader[1] << 8);
    
    /* Reset file pointer */
    rewind(input);

    buf = (unsigned char*) malloc(headerSize);
    if( buf == NULL ) {
        fprintf(stderr, "not enough memory");
        goto clean;
    }

    ret = fread(buf, 1, headerSize, input);
    if( ferror(input) ) {
        fprintf(stderr, "error reading input file %s: %s", input_file, strerror(errno));
        goto clean;
    }

    ret = fwrite(buf, 1, headerSize, output);
    if( ferror(output) ) {
        fprintf(stderr, "error writing output file %s: %s", output_file, strerror(errno));
        goto clean;
    }

    /* Jump to the data (Skip CRC32) */
    if( fseek(input, headerSize + 4, SEEK_SET) ) {
        ret = errno;
        goto clean;
    }

    /* decompress */
    ret = blast(inf, input, outf, output);
    switch (ret)
    {
    case 2:
        fprintf(stderr, "ran out of input before completing decompression");
        goto clean;
    case 1:
        fprintf(stderr, "output error before completing decompression");
        goto clean;
    case -1:
        fprintf(stderr, "literal flag not zero or one");
        goto clean;
    case -2:
        fprintf(stderr, "dictionary size not in 4..6");
        goto clean;
    case -3:
        fprintf(stderr, "distance is too far back");
        goto clean;
    default:
        break;
    }

    /* see if there are any leftover bytes */
    int n = 0;
    while (fgetc(input) != EOF) n++;
    if (n) {
        fprintf(stderr, "there are %d leftover bytes from decompression", n);
        ret = -1;
        goto clean;
    }

    ret = 0;

clean:
    if(input) fclose(input);
    if(output) fclose(output);
    if(buf) free(buf);
    return ret;
}

