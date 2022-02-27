import pandas as pd
import numpy as np
import struct, sys

COLS = ["ack","sec","ax","ay","az","gx","gy","gz","pitch","roll","yaw","ch1","ch2","co1","co2","pad"]
NCOLS = len(COLS)

src = sys.argv[1]
dst = sys.argv[1].replace(".bin",".csv")

with open(src,"rb") as f:
  df = pd.DataFrame(columns=COLS)
  n = 0
  while True:
    buf = f.read(4*NCOLS)
    if len(buf) > 0:
      row = struct.unpack("i"*2+"f"*(NCOLS-2), buf)
      df.loc[n] = row
      n = n + 1
    else:
      break
  df["sec"] = df["sec"]/1000.	#convert msec to sec
  df.to_csv(dst,index=False)
  print("\nconverted", src, "to", dst, "[",NCOLS,"x",n,"]")

