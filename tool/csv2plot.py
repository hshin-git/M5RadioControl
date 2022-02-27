import pandas as pd
import numpy as np
import struct, sys
import matplotlib.pyplot as plt

RENAME = {}

try:
  src = sys.argv[1]
  dst = sys.argv[1].replace(".csv","-plot.png")
  df = pd.read_csv(src,index_col="sec")
  df = df.rename(columns=RENAME)
except OSError as e:
  print(e)
else:
  ## summary
  NR = len(df)
  TD = df.index[-1] - df.index[0]
  COLS = df.columns[1:-1]
  print("NR:","%d(samples)"%NR)
  print("TD:","%.1f(sec)"%TD)
  print("CL:",COLS)
  ## plot
  LOC = "upper right"
  LABEL = ["accl (G)", "gyro (dps)", "ahrs (deg)", "pwm (usec)"]
  NP = len(LABEL)
  #
  fig,axs = plt.subplots(NP,1,sharex=True)
  for n in range(0,NP):
    pl = axs[n]
    if n==0: pl.set_title("Samples(%.1fsec@%.fHz): "%(TD,NR/TD) + src)
    for c in COLS[3*n:3*(n+1)]: df[c].plot(ax=pl,label=c)
    pl.set_ylabel(LABEL[n])
    pl.legend(loc=LOC)
    pl.grid()
  #
  plt.savefig(dst)

