#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include "spine_hal.h"
#include "spine_protocol.h"


const SpineMessageHeader* hal_read_frame();


static long getBaudrate(int baudrate)
{
	switch(baudrate) {
	case 0: return B0;
	case 50: return B50;
	case 75: return B75;
	case 110: return B110;
	case 134: return B134;
	case 150: return B150;
	case 200: return B200;
	case 300: return B300;
	case 600: return B600;
	case 1200: return B1200;
	case 1800: return B1800;
	case 2400: return B2400;
	case 4800: return B4800;
	case 9600: return B9600;
	case 19200: return B19200;
	case 38400: return B38400;
	case 57600: return B57600;
	case 115200: return B115200;
	case 230400: return B230400;
	default: return baudrate;
	}
}


void ctrlc_handler(int sig){
   //Just quit on sigints
   exit(-1);
}



static PyObject* spine_init(PyObject *self, PyObject* args)
{
   const char* path;
   long baudrate;
   long speed;

   signal(SIGINT, ctrlc_handler);
   signal(SIGKILL, ctrlc_handler);
   
   if (!PyArg_ParseTuple(args, "sl", &path, &baudrate)) { return NULL; }
   speed = getBaudrate(baudrate);
   SpineErr result = hal_init(path, speed);
   return Py_BuildValue("i", result);

}

static PyObject* spine_read_frame(PyObject *self, PyObject* args)
{
   if (!PyArg_ParseTuple(args, "")) { return NULL; } 
   const SpineMessageHeader *result = hal_read_frame();
   return Py_BuildValue("s#", result, sizeof(SpineMessageHeader)+result->bytes_to_follow);
}

static PyObject* spine_send_frame(PyObject *self, PyObject* args)
{
   PayloadId type;
   const char* buf;
   Py_ssize_t len;
   if (!PyArg_ParseTuple(args, "is#", &type, &buf, &len)) { return NULL; }
   hal_send_frame(type, buf, len);
   return Py_BuildValue(""); //return None
}


static PyMethodDef SpineMethods[] = {
   {"init", spine_init, METH_VARARGS, "initialize the spine hal"},
   {"read_frame", spine_read_frame, METH_VARARGS, "read the next full frame"},
   {"send_frame", spine_send_frame, METH_VARARGS, "send a payload of given type"},
   {NULL, NULL, 0, NULL}
};


static struct PyModuleDef spinemodule = {
   PyModuleDef_HEAD_INIT,
   "spine",   /* name of module */
   NULL, /* module documentation, may be NULL */
   -1,       /* size of per-interpreter state of the module,
                or -1 if the module keeps state in global variables. */
   SpineMethods
};

PyMODINIT_FUNC
PyInit_spine(void)
{
    return PyModule_Create(&spinemodule);
}

/*
 gcc -I~/code/adam/cpython/Include spinemodule.c 
*/

