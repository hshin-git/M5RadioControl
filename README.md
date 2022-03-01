# M5RadioControl
DIY Radio Control system by M5Stack

![IMG_RC_SYSTEM](https://user-images.githubusercontent.com/64751855/154823502-b6f40bb0-9fc7-4578-9e62-ac9f5db41d3a.jpg)

- M5RadioControl（M5RC）は、RCプロポの送受信機TX/RXをM5Stackで自作するためのオープンソースソフト（OSS）です。
- RCプロポの標準機能（ペアリング、モデルメモリ、トリム、リバース、D/R、EPA、EXP等）に加えて独自機能を実現しています。
- 独自機能は、「ダンシングライダー」気分の操縦機能、「走る実験室」気分のテレメータ／ロギング機能等です。
- 送信機TXは「M5Stack Gray」、受信機RXは「M5Atom Matrix」と「M5StickC」で動作確認済みです。
- TX/RX間の通信は、「ESP-NOW」を利用して、下り（TXからRXへ）約100Hz、上り（RXからTXへ）約300Hz、到達距離20m程度です。


# DEMO

M5RCシステムをタミヤ製のRCカー（グラスホッパー２）とミニ四駆（FM-Aシャーシ）へ搭載した事例です。

![M5RC_GR02](https://user-images.githubusercontent.com/64751855/155876897-721a2c08-705e-47fc-a46e-67b262cabae8.jpg)
![M5RC_M4WD](https://user-images.githubusercontent.com/64751855/155876951-76d9f351-90a1-456f-93d7-3befb422dc33.jpg)


# Features
~~退屈な~~標準機能の解説をすっ飛ばして、独自機能のロマン溢れる使い方を解説します。

## ダンシング操縦機能
送信機TXのグリップ自体を「操縦桿」の様に傾けることでRCカーを操縦できます。
例えばタミヤT3-01を操縦すると、貴方は「ダンシングライダー（オジサン）」の世界に没入できます。

![T3-01](https://d7z22c0gz59ng.cloudfront.net/japan_contents/img/usr/item/5/57405/57405_1.jpg)


## テレメータ／ロギング機能
RCカー搭載IMU（慣性計測ユニット）のテレメータ／ロギングにより、貴方専用の「走る実験室」をお手軽に実現できます。
走行データの定量分析により、貴方は「ホンダのF-1エンジニア」気分でデータドリブンにRCカーをチューニングできます。

![M4WD-plot](https://user-images.githubusercontent.com/64751855/155877157-9e4e1bb6-cacd-4e34-a1aa-a5ffe0449518.png)
![M4WD-traj](https://user-images.githubusercontent.com/64751855/156074555-eef4edac-f4a1-41a3-a283-c758bf34b154.png)

こちらはミニ四駆コース＠[RCカー練習場「元気っ子さん」](https://genkikkosan.com/)の走行データ例（三週目でコースアウト）です。
IMUデータは、シャーシ固定系に関する成分表示で、右方がx成分、前方がy成分、上方がz成分です。
3D軌跡は、車速vyがスロットル（ch2）に比例すると仮定してAHRS計算値（pitch、yaw）を積算した結果です。
センサ値に少しバイアスが乗っていますが、走行データから三次元的なコースレイアウトが分かります。


## ステアリング・ジャイロ機能
RCカーのステアリング操作をPIDコントローラが素早くアシストしてスピンを防ぎます。
例えばヨコモYD-2に搭載すると、貴方は安全かつ地球に優しく「頭文字Ｄ」の世界に浸れます。

![M5RC_DRIFT](https://user-images.githubusercontent.com/64751855/156068585-76c348eb-bc47-495f-889b-ec987f2f0023.jpg)

ソースコードは、こちらの[ラジドリ用ジャイロGyroM5](https://github.com/hshin-git/GyroM5)を流用しています。
PIDコントローラのパラメータ調整は、プロポ（送信機）側メニューから設定して即反映というお手軽さです。


## ドレミファ・インバータ機能
ブラシモーターのスイッチング周波数をタイマとスロットルに応じてプログラムできます。
例えばVVVFインバータ調サウンドにプログラムすると、貴方は電車運転士の気分です。

![RAILWAY_HDR](https://user-images.githubusercontent.com/64751855/156074703-8c3c4c0f-50f9-492a-83f9-2223110b4df2.jpg)



# Requirement

- 送信機TXハードウェアは、「M5Stack Gray」と「A/Dコンバータ(I2C接続)」
- 受信機RXハードウェアは、「M5Atom Matrix」または「M5StickC」
- ホビー用RCカー、サーボとESC（電子式スピードコントローラ）など

![M5RC-System](https://user-images.githubusercontent.com/64751855/156155080-d2c6f90a-a046-4abf-87f2-2449d05977ad.png)



# Usage
多機能なソフトウェアなので、ぼちぼち追記します。



# Reference

- [ラジドリ用ジャイロGyroM5＠github](https://github.com/hshin-git/GyroM5)
- [ラジドリ用ジャイロGyroM5＠protopedia](https://protopedia.net/prototype/2351)
- [3000円で至高の二駆ドリジャイロをゲットできたけど少し反省した話＠note](https://note.com/nanami00/n/n3a1958d79433)




