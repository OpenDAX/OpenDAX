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

static PyObject *pydax_add(PyObject *pSelf, PyObject *pArgs)
{
  PyObject *pX, *pY;
  if(!PyArg_ParseTuple(pArgs, "OO", &pX, &pY)) return NULL;
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
  {"add", pydax_add, METH_VARARGS, NULL},
  {"test", pydax_test, METH_VARARGS, NULL},
  {NULL, NULL}};

PyMODINIT_FUNC
initpydax(void)
{
  Py_InitModule("pydax", pydax_methods);
}
