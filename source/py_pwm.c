/*
Copyright (c) 2013 Ben Croston

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include "Python.h"
#include "soft_pwm.h"
#include "py_pwm.h"
#include "common.h"
#include "c_gpio.h"

#include "bcm2835.h"

#include <string.h>

typedef struct
{
    PyObject_HEAD
    unsigned int gpio;
    float freq;
    float dutycycle;
} PWMObject;


typedef struct
{
    PyObject_HEAD
    unsigned int gpio;
    unsigned int channel;
    float freq;
    unsigned int divider;
    unsigned int range;
} PWM2835Object;
 
// python method PWM.__init__(self, channel, frequency)
static int PWM_init(PWMObject *self, PyObject *args, PyObject *kwds)
{
    int channel;
    float frequency;

    if (!PyArg_ParseTuple(args, "if", &channel, &frequency))
        return -1;

    // convert channel to gpio
    if (get_gpio_number(channel, &(self->gpio)))
        return -1;

    // ensure channel set as output
    if (gpio_direction[self->gpio] != OUTPUT)
    {
        PyErr_SetString(PyExc_RuntimeError, "You must setup() the GPIO channel as an output first");
        return -1;
    }

    if (frequency <= 0.0)
    {
        PyErr_SetString(PyExc_ValueError, "frequency must be greater than 0.0");
        return -1;
    }

    self->freq = frequency;

    pwm_set_frequency(self->gpio, self->freq);
    return 0;
}

// python method PWM.start(self, dutycycle)
static PyObject *PWM_start(PWMObject *self, PyObject *args)
{
    float dutycycle;

    if (!PyArg_ParseTuple(args, "f", &dutycycle))
        return NULL;

    if (dutycycle < 0.0 || dutycycle > 100.0)
    {
        PyErr_SetString(PyExc_ValueError, "dutycycle must have a value from 0.0 to 100.0");
        return NULL;
    }

    self->dutycycle = dutycycle;
    pwm_set_duty_cycle(self->gpio, self->dutycycle);
    pwm_start(self->gpio);
    Py_RETURN_NONE;
}

// python method PWM.ChangeDutyCycle(self, dutycycle)
static PyObject *PWM_ChangeDutyCycle(PWMObject *self, PyObject *args)
{
    float dutycycle = 0.0;
    if (!PyArg_ParseTuple(args, "f", &dutycycle))
        return NULL;

    if (dutycycle < 0.0 || dutycycle > 100.0)
    {
        PyErr_SetString(PyExc_ValueError, "dutycycle must have a value from 0.0 to 100.0");
        return NULL;
    }

    self->dutycycle = dutycycle;
    pwm_set_duty_cycle(self->gpio, self->dutycycle);
    Py_RETURN_NONE;
}

// python method PWM. ChangeFrequency(self, frequency)
static PyObject *PWM_ChangeFrequency(PWMObject *self, PyObject *args)
{
    float frequency = 1.0;

    if (!PyArg_ParseTuple(args, "f", &frequency))
        return NULL;

    if (frequency <= 0.0)
    {
        PyErr_SetString(PyExc_ValueError, "frequency must be greater than 0.0");
        return NULL;
    }

    self->freq = frequency;

    pwm_set_frequency(self->gpio, self->freq);
    Py_RETURN_NONE;
}

// python function PWM.stop(self)
static PyObject *PWM_stop(PWMObject *self, PyObject *args)
{
    pwm_stop(self->gpio);
    Py_RETURN_NONE;
}

// deallocation method
static void PWM_dealloc(PWMObject *self)
{
    pwm_stop(self->gpio);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyMethodDef
PWM_methods[] = {
   { "start", (PyCFunction)PWM_start, METH_VARARGS, "Start software PWM\ndutycycle - the duty cycle (0.0 to 100.0)" },
   { "ChangeDutyCycle", (PyCFunction)PWM_ChangeDutyCycle, METH_VARARGS, "Change the duty cycle\ndutycycle - between 0.0 and 100.0" },
   { "ChangeFrequency", (PyCFunction)PWM_ChangeFrequency, METH_VARARGS, "Change the frequency\nfrequency - frequency in Hz (freq > 1.0)" },
   { "stop", (PyCFunction)PWM_stop, METH_VARARGS, "Stop software PWM" },
   { NULL }
};


PyTypeObject PWMType = {
   PyVarObject_HEAD_INIT(NULL,0)
   "RPi.GPIO.PWM",            // tp_name
   sizeof(PWMObject),         // tp_basicsize
   0,                         // tp_itemsize
   (destructor)PWM_dealloc,   // tp_dealloc
   0,                         // tp_print
   0,                         // tp_getattr
   0,                         // tp_setattr
   0,                         // tp_compare
   0,                         // tp_repr
   0,                         // tp_as_number
   0,                         // tp_as_sequence
   0,                         // tp_as_mapping
   0,                         // tp_hash
   0,                         // tp_call
   0,                         // tp_str
   0,                         // tp_getattro
   0,                         // tp_setattro
   0,                         // tp_as_buffer
   Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, // tp_flag
   "Pulse Width Modulation class",    // tp_doc
   0,                         // tp_traverse
   0,                         // tp_clear
   0,                         // tp_richcompare
   0,                         // tp_weaklistoffset
   0,                         // tp_iter
   0,                         // tp_iternext
   PWM_methods,               // tp_methods
   0,                         // tp_members
   0,                         // tp_getset
   0,                         // tp_base
   0,                         // tp_dict
   0,                         // tp_descr_get
   0,                         // tp_descr_set
   0,                         // tp_dictoffset
   (initproc)PWM_init,        // tp_init
   0,                         // tp_alloc
   0,                         // tp_new
};

PyTypeObject *PWM_init_PWMType(void)
{
   // Fill in some slots in the type, and make it ready
   PWMType.tp_new = PyType_GenericNew;
   if (PyType_Ready(&PWMType) < 0)
      return NULL;

   return &PWMType;
}

// python method PWM.__init__(self, pwm_channel, gpio,  diviser, range)
static int PWM2835_init(PWM2835Object *self, PyObject *args, PyObject *kwds)
{
    unsigned int pwm_channel, gpio;
    unsigned int divider;
    unsigned int range;
    if (!PyArg_ParseTuple(args, "IIII", &pwm_channel, &gpio, &divider, &range)) {
        PyErr_SetString(PyExc_ValueError, "Error in parameters");
        return -1;
    }
    
//    divider = pow((int) (log(divider) / log(2)), 2);
    self->gpio = gpio;
    self->divider = divider;
    self->range = range;
    self->channel = pwm_channel;
    self->freq = 19200000 / divider / range;

    init_pwm(gpio, pwm_channel, divider, range);
    printf("PWM2835 init : gpio %d, channel : %d, frequence : %f Hz, divider : %d, range : %d\n", self->gpio, self->channel, self->freq, self->divider, self->range);
    return 0;
}

// python method PWM2835.setClock(divider)
static PyObject *PWM2835_SetClock(PWM2835Object *self, PyObject *args)
{
    unsigned int divider;

    if (!PyArg_ParseTuple(args, "i", &divider)) {
        PyErr_SetString(PyExc_ValueError,  " Error divider parameter");
        return NULL;
    }
    pwm_setclock(divider);
    self->divider = divider;
    self->freq = 19200000 / self->divider / self->range;
    Py_RETURN_NONE;
}

// python method PWM2835.setRange(range)
static PyObject *PWM2835_SetRange(PWM2835Object *self, PyObject *args)
{
    unsigned int range;

    if (!PyArg_ParseTuple(args, "i", &range)) {
        PyErr_SetString(PyExc_ValueError,  " Error range parameter");
        return NULL;
    }
    pwm_setrange(self->channel, range);
    self->range = range;
    self->freq = 19200000 / self->divider / self->range;
    Py_RETURN_NONE;
}

// python method PWM2835.GetFrequence()
static PyObject *PWM2835_GetFrequence(PWM2835Object *self, PyObject *args)
{
    return Py_BuildValue("f", self->freq);
}

// python method PWM2835.setLevel(level)
static PyObject *PWM2835_SetLevel(PWM2835Object *self, PyObject *args)
{
    unsigned int level, range;

    if (!PyArg_ParseTuple(args, "i", &level)) {
        PyErr_SetString(PyExc_ValueError,  " Error level parameter");
        return NULL;
    }
    
    range = (unsigned int) (self->range * (level / 100.0));

    pwm_setlevel(self->channel, range);
    Py_RETURN_NONE;
}

// python method PWM.sendPulsePairs(self, PulsePairsTab, Level)
static PyObject *PWM2835_sendPulsePairs(PWM2835Object *self, PyObject *args)
{
    int i, size, pairsize;
    float level = 100.0;
    unsigned int range = level;
    int ** pulsepairs;
    PulsePair pair;

    PyObject *tab;
    PyObject *item;
    long pulse, pause;
    
    printf("****** sendPulsePairs ******\n");
    if (!PyArg_ParseTuple(args, "O!f", &PyList_Type, &tab, &level)) {
        printf("--- error parsed tuple");
        PyErr_SetString(PyExc_ValueError,  " +++ error parsetuple");
        return NULL;
    }
    if (level < 0.0 || level > 100.0)
    {
        PyErr_SetString(PyExc_ValueError,"Level must have a value from 0.0 to 100.0\% of range.");
        return NULL;
    }

    range = (unsigned int) ((float)self->range * (level / 100.0));
    
    if (!PyList_Check(tab)) {
        printf("--- Not a list\n");
        if (!PyDict_Check(tab)) printf("--- Not a Dict\n");
        return NULL;
    }
    size = PyList_Size(tab);
    printf("List pulse pair size : %d, range : %i on gpio : %d, channel : %d\n", size, range, self->gpio,self->channel);
    if (size < 0) { /* Not a list */
        PyErr_SetString(PyExc_ValueError, "Empty pulse / pairs table");
        return NULL; 
    }
    pulsepairs = malloc(sizeof(int *) * size);
    pairsize = sizeof(pulsepairs);
    for (i = 0; i < size; i++) {
        item = PyList_GetItem(tab, i); /* Can't fail */
        pairsize = PyList_Size(item);
        if (!PyList_Check(item)) { /* Skip list object */
            PyErr_SetString(PyExc_ValueError, "Not a list pulse pair format.");
            return NULL;
        }
        if (pairsize != 2) {
            PyErr_SetString(PyExc_ValueError, "Not a pulse pair format.");
            return NULL;
        }
        pulse = PyInt_AsLong(PyList_GetItem(item, 0));
        pause = PyInt_AsLong(PyList_GetItem(item, 1));
        pulsepairs[i] = malloc(sizeof(long *) * 2);
        pulsepairs[i][0] = pulse;
        pulsepairs[i][1] = pause;
    }
    printf("data formaté\n");
    pwm_setlevel(self->channel, range);
    for (i = 0; i < size; i++) {
        pwm_pulsepause(self->channel, pulsepairs[i][0], pulsepairs[i][1], (int)range, &pair);
 //       printf("code %d : pulse %ld us (%ld), pause %ld us (%ld)\n", i, pair.pulse, pulsepairs[i][0], pair.pause,  pulsepairs[i][1]);
        pulsepairs[i][0] = pair.pulse;
        pulsepairs[i][1] = pair.pause;
    }
    
    PyObject *Result=PyList_New(size);
    PyObject *ItemR=PyList_New(2);
    
    for (i = 0; i < size; i++) {
        PyList_SetItem(ItemR, 0, PyInt_FromLong(pulsepairs[i][0]));
        PyList_SetItem(ItemR, 1, PyInt_FromLong(pulsepairs[i][1]));
        PyList_SetItem(Result, i, PyList_AsTuple(ItemR));
    }
    for (i = 0; i < size; i++) {
        free(pulsepairs[i]);
    }

    free(pulsepairs);
    pulsepairs = NULL;
    return Result;
}

// deallocation method
static void PWM2835_dealloc(PWM2835Object *self)
{
    close_bcm2835();
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyMethodDef
PWM2835_methods[] = {
   { "SetClock", (PyCFunction)PWM2835_SetClock, METH_VARARGS, "Set clock diviser." },
   { "SetRange", (PyCFunction)PWM2835_SetRange, METH_VARARGS, "Set range." },
   { "SetLevel", (PyCFunction)PWM2835_SetLevel, METH_VARARGS, "Set the level (0.0 to 100.0\% of range)." },
   { "GetFrequence", (PyCFunction)PWM2835_GetFrequence, METH_VARARGS, "Set the level (0.0 to 100.0\% of range)." },
   { "SendPulsePairs",(PyCFunction)PWM2835_sendPulsePairs, METH_VARARGS, "Start PWM for a Pulse/Pause pairs tab - the level (0.0 to 100.0\% of range)"},
   { NULL }
};


PyTypeObject PWM2835Type = {
   PyVarObject_HEAD_INIT(NULL,0)
   "RPi.GPIO.PWM2835",            // tp_name
   sizeof(PWM2835Object),         // tp_basicsize
   0,                         // tp_itemsize
   (destructor)PWM2835_dealloc,   // tp_dealloc
   0,                         // tp_print
   0,                         // tp_getattr
   0,                         // tp_setattr
   0,                         // tp_compare
   0,                         // tp_repr
   0,                         // tp_as_number
   0,                         // tp_as_sequence
   0,                         // tp_as_mapping
   0,                         // tp_hash
   0,                         // tp_call
   0,                         // tp_str
   0,                         // tp_getattro
   0,                         // tp_setattro
   0,                         // tp_as_buffer
   Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, // tp_flag
   "Pulse Width Modulation using BCM2835 Hard",    // tp_doc
   0,                         // tp_traverse
   0,                         // tp_clear
   0,                         // tp_richcompare
   0,                         // tp_weaklistoffset
   0,                         // tp_iter
   0,                         // tp_iternext
   PWM2835_methods,               // tp_methods
   0,                         // tp_members
   0,                         // tp_getset
   0,                         // tp_base
   0,                         // tp_dict
   0,                         // tp_descr_get
   0,                         // tp_descr_set
   0,                         // tp_dictoffset
   (initproc)PWM2835_init,        // tp_init
   0,                         // tp_alloc
   0,                         // tp_new
};

PyTypeObject *PWM2835_init_PWMType(void)
{
   // Fill in some slots in the type, and make it ready
   PWM2835Type.tp_new = PyType_GenericNew;
   if (PyType_Ready(&PWM2835Type) < 0)
      return NULL;

   return &PWM2835Type;
}
