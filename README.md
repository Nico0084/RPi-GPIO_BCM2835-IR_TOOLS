RPi-GPIO_BCM2835-IR_TOOLS
=========================

Librairie RPi.GPIO etendue avec la lib BCM2835 pour gestion hardware du PWM

* Ajout de fonctions BCM* dans `source/py_gpio.c`, Accès par RPi.GPIO.BCM.....
* Ajout de l'objet pwm hardware (accès directe aux registres mémoire) dans `source/py_pwm.c` RPio.GPIO.PWM2835....
* Implémentation de gestion d'envois de pairs pulse/pause pour code infrarouge.
 
Consulter le projet https://github.com/Nico0084/RPi-IRTransceiver-WS-Server pour voir l'utilisation de la lib.
notament dans le fichier `/lib/rpi_irtrans.py`

Installation :
-------------

    git clone https://github.com/Nico0084/RPi-GPIO_BCM2835-IR_TOOLS.git


    sudo python setup.py install
Ou

    sudo python3 setup.py install
