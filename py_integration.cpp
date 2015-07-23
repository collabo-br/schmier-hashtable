#include "htb64.h"
#include "one_at_time.hpp"
#include "ht_file_versioning.h"

#include "htb64.cpp"
#include "ht_file_versioning.cpp"

extern "C" {

#include <Python.h>
#include "structmember.h"

typedef struct {
    PyObject_HEAD
    HTFileVersioning *fv;
} PY_HTFileVersioning;

static void
PY_HTFileVersioning_dealloc(PY_HTFileVersioning* self)
{
    if (!self) return;
    delete self->fv;
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject *
PY_HTFileVersioning_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    PY_HTFileVersioning *self;

    self = (PY_HTFileVersioning *)type->tp_alloc(type, 0);
    if (self != NULL) {
        self->fv = new HTFileVersioning();
    }

    return (PyObject *)self;
}

static int
PY_HTFileVersioning_init(PY_HTFileVersioning *self, PyObject *args, PyObject *kwds)
{
    self->fv->reset();
    return 0;
}

static PyObject *
PY_HTFileVersioning_reset(PY_HTFileVersioning* self)
{
    self->fv->reset();
    Py_RETURN_NONE;
}

static PyObject *
PY_HTFileVersioning_addFile(PY_HTFileVersioning* self, PyObject *args)
{
    PyObject* fnamel;
    if (!PyArg_ParseTuple(args, "O", &fnamel)) {
        return NULL;
    }

    if (PyString_Check(fnamel))
    {
        char *string = PyString_AS_STRING(fnamel);
        self->fv->addFile(string);
        Py_RETURN_NONE;
    }

    if (!PyIter_Check(fnamel))
    {
        PyObject *iterator = PyObject_GetIter(fnamel);
        PyObject *item;

        if (!iterator)
            return NULL;

        while ( (item = (PyIter_Next(iterator))) )
        {
            if(!PyString_Check(item))
            {
                Py_DECREF(item);
                return NULL;
            }

            char *string = PyString_AS_STRING(item);
            self->fv->addFile(string);

            Py_DECREF(item);
        }

        Py_DECREF(iterator);
        Py_RETURN_NONE;
    }

    return NULL;
}

static PyObject *
PY_HTFileVersioning_checkFile(PY_HTFileVersioning* self, PyObject *args)
{
    PyObject* fnamel;
    if (!PyArg_ParseTuple(args, "O", &fnamel)) {
        return NULL;
    }

    if (PyString_Check(fnamel))
    {
        char *string = PyString_AS_STRING(fnamel);

        if (self->fv->checkFile(string))
            Py_RETURN_TRUE;
        else
            Py_RETURN_FALSE;
    }

    return NULL;
}

static PyObject *
PY_HTFileVersioning_getHTable(PY_HTFileVersioning* self)
{
    return PyString_FromString(self->fv->getHTable().c_str());
}

static PyObject *
PY_HTFileVersioning_setHTable(PY_HTFileVersioning* self, PyObject *args)
{
    PyObject* fnamel;
    if (!PyArg_ParseTuple(args, "O", &fnamel)) {
        return NULL;
    }

    if (PyString_Check(fnamel))
    {
        self->fv->setHTable(
            std::string(PyString_AS_STRING(fnamel))
        );

        Py_RETURN_NONE;
    }

    return NULL;
}

static PyMethodDef PY_HTFileVersioning_methods[] = {
    {
        "reset", (PyCFunction)PY_HTFileVersioning_reset, METH_NOARGS,
        "Clears the internal state of the object"
    }, {
        "add_file", (PyCFunction)PY_HTFileVersioning_addFile, METH_VARARGS,
        "Add a file or a list of files to the hashmap"
    }, {
        "check_file", (PyCFunction)PY_HTFileVersioning_checkFile, METH_VARARGS,
        "Checks if file is in the hashmap"
    }, {
        "get_table", (PyCFunction)PY_HTFileVersioning_getHTable, METH_NOARGS,
        "Get a B64 encoded hash table"
    }, {
        "set_table", (PyCFunction)PY_HTFileVersioning_setHTable, METH_VARARGS,
        "Set a B64 encoded hash table"
    },
    {NULL}  /* Sentinel */
};

static PyMemberDef PY_HTFileVersioning_members[] = {
    {NULL}  /* Sentinel */
};

static PyTypeObject PY_HTFileVersioningType = {
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "FileVersioning",       /*tp_name*/
    sizeof(PY_HTFileVersioning), /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)PY_HTFileVersioning_dealloc, /*tp_dealloc*/
    0,                         /*tp_print*/
    0,                         /*tp_getattr*/
    0,                         /*tp_setattr*/
    0,                         /*tp_compare*/
    0,                         /*tp_repr*/
    0,                         /*tp_as_number*/
    0,                         /*tp_as_sequence*/
    0,                         /*tp_as_mapping*/
    0,                         /*tp_hash */
    0,                         /*tp_call*/
    0,                         /*tp_str*/
    0,                         /*tp_getattro*/
    0,                         /*tp_setattro*/
    0,                         /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /*tp_flags*/
    "FileVersioning generates a hashtable "
    "allowing to check if file have been hashed", /* tp_doc */
    0,                     /* tp_traverse */
    0,                     /* tp_clear */
    0,                     /* tp_richcompare */
    0,                     /* tp_weaklistoffset */
    0,                     /* tp_iter */
    0,                     /* tp_iternext */
    PY_HTFileVersioning_methods, /* tp_methods */
    PY_HTFileVersioning_members, /* tp_members */
    0,                         /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    (initproc)PY_HTFileVersioning_init, /* tp_init */
    0,                         /* tp_alloc */
    PY_HTFileVersioning_new,   /* tp_new */
};

static PyMethodDef module_methods[] = {
    {NULL}  /* Sentinel */
};


void initht_file(void)
{
    PyObject* m;

    if (PyType_Ready(&PY_HTFileVersioningType) < 0)
        return;

    m = Py_InitModule3("ht_file", module_methods,
                       "Provides FileVersioning");

    if (m == NULL)
      return;

    Py_INCREF(&PY_HTFileVersioningType);
    PyModule_AddObject(m, "FileVersioning", (PyObject *)&PY_HTFileVersioningType);
}

}; // END extern "C"
