import numpy as np

from dac4813 import DAC4813

with DAC4813("/dev/ttyUSB0") as dac:
    print(dac.identify())
    while True:
        for i in np.arange(4096):
            print(i)
            dac.set(1, i)

