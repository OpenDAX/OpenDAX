/*  PyDAX - A Python extension module for OpenDAX 
 *  OpenDAX - An open source data acquisition and control system 
 *  Copyright (c) 2011 Phil Birkelbach
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 * 
 *  This is the main source code file for the extension module
 */

#include <pydax.h>

/* TODO: This should eventually be a Python object that we
 * create and handle.  Right now we'll keep it a global for
 * simplicity but it will only allow one connection per
 * Python script. */
dax_state *ds;

static PyObject *pydax_init(PyObject *pSelf, PyObject *pArgs)
{
    char *modname;
    int result;
    /* TODO: Get command line arguments and process them too */
    int argc = 1;
    char *argv[] = {modname};
    
    if(!PyArg_ParseTuple(pArgs, "s", &modname)) return NULL;

    ds = dax_init(modname);
    if(ds == NULL) {
        PyErr_SetString(PyExc_IOError, "Unable to Allocate Dax State Object");
        return NULL;
    }
    /* Initialize and run the configuration */
    result = dax_init_config(ds, modname);
    if(result != 0) {
        PyErr_SetString(PyExc_IOError, "Unable to Allocate Configuration");
        return NULL;
    }
    dax_configure(ds, argc, argv, CFG_CMDLINE | CFG_DAXCONF);

    /* Free the configuration data */
    dax_free_config(ds);

    /* Check for OpenDAX and register the module */
    if( dax_mod_register(ds) ) {
        PyErr_SetString(PyExc_IOError, "Unable to Connect to OpenDAX Server");
        return NULL;
    }

    Py_RETURN_NONE;
}


static PyObject *pydax_add(PyObject *pSelf, PyObject *pArgs)
{
    char *tagname, *type;
    int count
    if(!PyArg_ParseTuple(pArgs, "ssi", &tagname, &type, &count)) return NULL;

    
    return PyNumber_Add(pX,pY);
}

struct Items {
    char *string;
    long num;
};


struct Items items[] = {{"Sup", 1000},
                        {"Dope", 2000},
                        {"Nope", 3050},
                        {"None", 0}, 
                        {NULL, 0}};

static PyObject *pydax_test(PyObject *pSelf, PyObject *pArgs)
{
    PyObject *o, *s, *v;
    int result, n=0;
  
    o = PyDict_New();
    if(o == NULL) return NULL;
    while(items[n].string != NULL) {
        s = PyString_FromString(items[n].string);
        v = PyInt_FromLong(items[n].num);
        result = PyDict_SetItem(o, s, v);
        Py_DECREF(s);
        Py_DECREF(v);
        n++;
    }
  
    return o;
}


static PyMethodDef pydax_methods[] = {
  {"init", pydax_init, METH_VARARGS, NULL},
  {"add", pydax_add, METH_VARARGS, NULL},
  {"test", pydax_test, METH_VARARGS, NULL},
  {NULL, NULL}};

PyMODINIT_FUNC
initpydax(void)
{
  Py_InitModule("pydax", pydax_methods);
}
