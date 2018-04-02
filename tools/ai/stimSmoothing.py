#!/usr/bin/env python3

import matplotlib.pyplot as plt

## fake data

stim = [ 0.0,
         0.0,
         0.0,
         0.0,
         0.0,
         0.6,
         0.6,
         0.55,
         0.5,
         0.48,
         0.46,
         0.7,
         0.7,
         0.65,
         0.6,
         0.55,
         0.5,
         0.49,
         0.47,
         0.45,
         0.43,
         0.41,
         0.4,
         0.39,
         0.38
]


def doplot(n):
    alpha = 2/(1+n)


    ma = []

    ema = 0.0

    for s in stim:
        ema = s*alpha + ema*(1-alpha)
        ma.append(ema)

    plt.figure()
    plt.title('N={}, alpha={}'.format(n, alpha))
    plt.plot(stim, 'r-')
    plt.plot(ma, 'g--')

doplot(1)
doplot(2)
doplot(3)
doplot(5)
doplot(10)
doplot(20)

plt.show()
