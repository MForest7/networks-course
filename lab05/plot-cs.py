import numpy as np
import matplotlib.pyplot as plt

F = 15 * 10 ** 9
us = 30 * 10 ** 6
d = 2 * 10 ** 6
N = [(10, "red"), (100, "green"), (1000, "blue")]
u = [3 * 10 ** 5, 7 * 10 ** 5, 20 * 10 ** 5]

for (n, color) in N:
    cs = lambda u: max(n * F / us, F / d)
    plt.plot(u, list(map(cs, u)), color=color, label=f'N = {n}')

plt.legend(loc='upper right')
plt.show()