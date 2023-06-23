/* decompress.c
   Copyright (C) 2023 Daniela Petruzalek

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

#define PY_SSIZE_T_CLEAN
#include <Python.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdint.h>

#include "blast.h"

#define CHUNK 4096

static PyObject *decompress(PyObject *self, PyObject *args);
static PyObject *read_dbc(PyObject *self, PyObject *args);

static PyObject *DatasusError;

static PyMethodDef DatasusMethods[] = {
    {"decompress",  decompress, METH_VARARGS, "Decompress a DATASUS *.dbc file into a *.dbf"},
    {"read_dbc",  read_dbc, METH_VARARGS, "Reads a DATASUS *.dbc file to memory"},
    {NULL, NULL, 0, NULL}        /* Sentinel */
};

static struct PyModuleDef datasusmodule = {
    PyModuleDef_HEAD_INIT,
    "datasus",   // name of module
    "tools for handling datasus (*.dbc) files", // module documentation, may be NULL
    -1,          // size of per-interpreter state of the module,
                 // or -1 if the module keeps state in global variables.
    DatasusMethods
};


PyMODINIT_FUNC
PyInit_datasus(void)
{
    PyObject *m;

    m = PyModule_Create(&datasusmodule);
    if (m == NULL)
        return NULL;

    DatasusError = PyErr_NewException("datasus.error", NULL, NULL);
    Py_XINCREF(DatasusError);
    if (PyModule_AddObject(m, "error", DatasusError) < 0) {
        Py_XDECREF(DatasusError);
        Py_CLEAR(DatasusError);
        Py_DECREF(m);
        return NULL;
    }

    return m;
}

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

static int _decompress(FILE* input, FILE* output) {
    unsigned char rawHeader[2];
    uint16_t      headerSize = 0;
    unsigned char *buf = 0;
    int           ret = -1;

    /* Process file header - skip 8 bytes */
    if( fseek(input, 8, SEEK_SET) ) {
        PyErr_SetFromErrno(DatasusError);
        goto _decompress_clean;
    }

    /* Reads two bytes from the header = header size */
    fread(rawHeader, 2, 1, input);
    if( ferror(input) ) {
        PyErr_SetFromErrno(DatasusError);
        goto _decompress_clean;
    }

    /* Platform independent code (header size is stored in little endian format) */
    headerSize = rawHeader[0] + (rawHeader[1] << 8);
    
    /* Reset file pointer */
    rewind(input);

    buf = (unsigned char*) malloc(headerSize);
    if( buf == NULL ) {
        PyErr_SetString(DatasusError, "not enough memory\n");
        goto _decompress_clean;
    }

    fread(buf, 1, headerSize, input);
    if( ferror(input) ) {
        PyErr_SetFromErrno(DatasusError);
        goto _decompress_clean;
    }

    fwrite(buf, 1, headerSize, output);
    if( ferror(output) ) {
        PyErr_SetFromErrno(DatasusError);
        goto _decompress_clean;
    }

    /* Jump to the data (Skip CRC32) */
    if( fseek(input, headerSize + 4, SEEK_SET) ) {
        PyErr_SetFromErrno(DatasusError);
        goto _decompress_clean;
    }

    /* decompress */
    ret = blast(inf, input, outf, output);
    if(ret != 0) {
        PyErr_SetString(DatasusError, "error decompressing file: make sure file is downloaded in binary mode and try again");
        goto _decompress_clean;
    }

    /* see if there are any leftover bytes */
    int n = 0;
    while (fgetc(input) != EOF) n++;
    if (n) {
        PyErr_SetString(DatasusError, "there are leftover bytes after decompression: check file integrity");
    }

_decompress_clean:
    if(buf) free(buf);
    return ret;
}


/*
    decompress:
    This function decompresses a given .dbc input file into the corresponding .dbf.

    Please provide fully qualified names, including file extension.
 */
static PyObject *
decompress(PyObject *self, PyObject *args) {
    FILE       *input = 0, *output = 0;
    const char *input_file;
    const char *output_file;
    int        result = -1;

    if (!PyArg_ParseTuple(args, "ss", &input_file, &output_file))
        return NULL;

    input = fopen(input_file, "rb");
    if(input == NULL) {
        PyErr_SetFromErrnoWithFilename(DatasusError, input_file);
        goto clean;
    }

    output = fopen(output_file, "wb");
    if(output == NULL) {
        PyErr_SetFromErrnoWithFilename(DatasusError, output_file);
        goto clean;
    }

    result = _decompress(input, output);

clean:
    if(input) fclose(input);
    if(output) fclose(output);

    return Py_BuildValue("i", result);
}


static PyObject *
read_dbc(PyObject *self, PyObject *args) {
    PyObject *result = NULL;
    const char* input_file;
    if (!PyArg_ParseTuple(args, "s", &input_file))
        return NULL;

    FILE *input = fopen(input_file, "rb");
    if(input == NULL) {
        PyErr_SetFromErrnoWithFilename(DatasusError, input_file);
        goto clean_read_dbc;
    }

    PyObject* tempfile = PyImport_ImportModule("tempfile");
    PyObject* mkstemp = PyObject_CallMethod(tempfile, "mkstemp", NULL);

    PyObject* output_file_desc = PyTuple_GetItem(mkstemp, 0);
    PyObject* output_filename = PyTuple_GetItem(mkstemp, 1);

    int output_fd = (int) PyLong_AsLong(output_file_desc);
    // PySys_WriteStdout("file descriptor: %d\n", output_fd);

    FILE* output = fdopen(output_fd, "wb");
    if(output == NULL) {
        PyErr_SetFromErrno(DatasusError);
        goto clean_read_dbc;
    }

    int ret = _decompress(input, output);
    if(ret != 0) {
        result = NULL;
        goto clean_read_dbc;
    }

    PyObject* dbf = PyImport_ImportModule("dbfread");
    PyObject* read_dbf = PyObject_CallMethod(dbf, "DBF", "O", output_filename);
    
    result = read_dbf;

clean_read_dbc:
    if(input) fclose(input);

    return result;
}
