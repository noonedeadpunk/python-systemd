#include <Python.h>
#define SD_JOURNAL_SUPPRESS_LOCATION
#include <systemd/sd-journal.h>

PyDoc_STRVAR(journald_sendv__doc__,
             "sendv('FIELD=value', 'FIELD=value', ...) -> None\n\n"
             "Send an entry to journald."
             );

static PyObject *
journald_sendv(PyObject *self, PyObject *args) {
    struct iovec *iov = NULL;
    int argc = PyTuple_Size(args);
    int i, r;
    PyObject *ret = NULL;

    PyObject **encoded = calloc(argc, sizeof(PyObject*));
    if (!encoded) {
        ret = PyErr_NoMemory();
        goto out1;
    }

    // Allocate sufficient iovector space for the arguments.
    iov = malloc(argc * sizeof(struct iovec));
    if (!iov) {
        ret = PyErr_NoMemory();
        goto out;
    }

    // Iterate through the Python arguments and fill the iovector.
    for (i = 0; i < argc; ++i) {
        PyObject *item = PyTuple_GetItem(args, i);
        char *stritem;
        Py_ssize_t length;

        if (PyUnicode_Check(item)) {
            encoded[i] = PyUnicode_AsEncodedString(item, "utf-8", "strict");
            if (encoded[i] == NULL)
                goto out;
            item = encoded[i];
        }
        if (PyBytes_AsStringAndSize(item, &stritem, &length))
            goto out;

        iov[i].iov_base = stritem;
        iov[i].iov_len = length;
    }

    // Clear errno, because sd_journal_sendv will not set it by
    // itself, unless an error occurs in one of the system calls.
    errno = 0;

    // Send the iovector to journald.
    r = sd_journal_sendv(iov, argc);

    if (r) {
        if (errno)
            PyErr_SetFromErrno(PyExc_IOError);
        else
            PyErr_SetString(PyExc_ValueError, "invalid message format");
        goto out;
    }

    // End with success.
    Py_INCREF(Py_None);
    ret = Py_None;

out:
    for (i = 0; i < argc; ++i)
        Py_XDECREF(encoded[i]);

    free(encoded);

out1:
    // Free the iovector. The actual strings
    // are already managed by Python.
    free(iov);

    return ret;
}

static PyMethodDef methods[] = {
    {"sendv",  journald_sendv, METH_VARARGS, journald_sendv__doc__},
    {NULL, NULL, 0, NULL}        /* Sentinel */
};

#if PY_MAJOR_VERSION < 3

PyMODINIT_FUNC
init_journald(void)
{
    (void) Py_InitModule("_journald", methods);
}

#else

static struct PyModuleDef module = {
    PyModuleDef_HEAD_INIT,
    "_journald", /* name of module */
    NULL, /* module documentation, may be NULL */
    0, /* size of per-interpreter state of the module */
    methods
};

PyMODINIT_FUNC
PyInit__journald(void)
{
    return PyModule_Create(&module);
}

#endif